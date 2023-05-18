#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>

//Aquí hacemos las funciones de la ALU
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    bitPos.push_back(1);

    for(int i = 1; i < 32; i++) bitPos.push_back(2*bitPos.at(i-1));

    ui->op1IEEE->setReadOnly(true);
    ui->op1Hex->setReadOnly(true);
    ui->op2IEEE->setReadOnly(true);
    ui->op2Hex->setReadOnly(true);
    ui->resulReal->setReadOnly(true);
    ui->resulIEEE->setReadOnly(true);
    ui->resulHex->setReadOnly(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}
/************************************ SUMA ************************************/
void MainWindow::on_suma_clicked()
{
    float op1 = ui->op1Real->text().toFloat();

    float op2 = ui->op2Real->text().toFloat();

    float salida = addOperation(op1,op2);

    /*Muestra el resultado en notación científica en los números grandes*/
    QString resultado;

    if (salida >= 1e7 || salida <= -10000000000){
        resultado = QString::number(salida, 'E', 0);

        if (salida >= 1e7) {
            resultado.replace("+", "");
        }
    }
    else {
        resultado = QString::number(salida, 'f');
    }

    binaryWriteIn( ui->op1IEEE, IEEE754Converter::floattoIEESign(op1), IEEE754Converter::floattoIEEExp(op1), IEEE754Converter::floattoIEEMantisa(op1));
    hexWriteIn(ui->op1Hex,  IEEE754Converter::floattoIEESign(op1), IEEE754Converter::floattoIEEExp(op1), IEEE754Converter::floattoIEEMantisa(op1));

    binaryWriteIn( ui->op2IEEE,  IEEE754Converter::floattoIEESign(op2), IEEE754Converter::floattoIEEExp(op2), IEEE754Converter::floattoIEEMantisa(op2));
    hexWriteIn(ui->op2Hex, IEEE754Converter::floattoIEESign(op2), IEEE754Converter::floattoIEEExp(op2), IEEE754Converter::floattoIEEMantisa(op2));

    ui->resulReal->setText(resultado);
    binaryWriteIn(ui -> resulIEEE, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
    hexWriteIn(ui->resulHex, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
}

float MainWindow::addOperation(float op1, float op2){
    //Pasos previos

    unsigned int signoA = IEEE754Converter::floattoIEESign(op1);
    unsigned int signoB = IEEE754Converter::floattoIEESign(op2);

    unsigned int expA = IEEE754Converter::floattoIEEExp(op1);
    unsigned int expB = IEEE754Converter::floattoIEEExp(op2);

    unsigned int manA = IEEE754Converter::floattoIEEMantisa(op1) + bitPos.at(23);
    unsigned int manB = IEEE754Converter::floattoIEEMantisa(op2) + bitPos.at(23);
    
    //1. Inicializamos los bits de guarda, sticky y round a 0. Además, inicializamos operandos intercambiados y complementado de P a falso
    unsigned int signoSuma;

    unsigned int g = 0; unsigned int r = 0; unsigned int st = 0;
    bool opChanged = false;
    bool compP = false;

    //2. Coomprobamos si el exponente de A es menor que el de B, en tal caso, se cambian de valores, pasando op2 a ser el op1, y el op1 a ser el op2.
    if(expA < expB){
        expA = IEEE754Converter::floattoIEEExp(op2);
        expB = IEEE754Converter::floattoIEEExp(op1);
        manA = IEEE754Converter::floattoIEEMantisa(op2) + bitPos.at(23);
        manB = IEEE754Converter::floattoIEEMantisa(op1) + bitPos.at(23);
        signoA = IEEE754Converter::floattoIEESign(op2);
        signoB = IEEE754Converter::floattoIEESign(op1);
        opChanged = true;
    }
    
    //3. El exponente de la suma se convierte ne el exponente de A y "d" es la resta entre ambos exponentes (expA - expB).
    int expR = expA;
    unsigned int d = expA - expB;
    
    //4. Comprobamos si los signos son diferentes, en tal caso, la mantisa de B se convierte en su complemento a 2.
    if(signoA!=signoB) manB = (~manB)%0b1000000000000000000000000+1;

    //5. Creamos P igualándolo a la mantisa de B.
    unsigned int P = manB;
    
    //6. Asignamos los bits de guarda, sticky y round solamente si d es mayor o igual a 3 y estrictamente menor que 25.
    //Ej: Si d = 7 y P = 010110000000000000000000 -> g = 0 (d-1=6, contamos 6 bits desde la DERECHA); el bit r la posición d-2, sticky OR de cada bit desde el bit d-3 hasta el bit 0
    if(d>=3 && d < 25){
        g = (bitPos.at(d-1)&P)!=0;
        r = (bitPos.at(d-2)&P)!=0;
        st = (bitPos.at(d-3)&P)!=0;
    }
    for(int i = d-3; i > 0 && !st; i--) st = st|(bitPos.at(d-i)&P); //El bit sticky realiza un OR para establecer su valor.
    
    //7. Comprobamos de nuevo si los signos son diferentes. En tal caso, desplazamos P a la derecha "d" bits introduciendo unos por la izquierda.
    if(signoA!=signoB){
        for(unsigned int i = 0; i < d; i++){
            P >>= 1;
            P += bitPos.at(23);
        }
    }else P = P>>d; //En caso de ser iguales desplazamos P a la derecha "d" bits introduciendo ceros por la izquierda.
    
    //8. Hacemos que P sea si misma más mantisa de A. El acarreo es el resultado de la suma entre ambas en caso de que haya habido un bit "1" adicional al principio.
    unsigned int C = 0;
    C = calcularAcarreo(manA, P, 0, 0);
    P = manA + P;

    //9. Comprobamos si los signos son diferentes, si el bit 24 - 1 es 1 y si el acarreo es 0.
    if(signoA!=signoB && (P & bitPos.at(23)) != 0 && C == 0){
        P = (~P)%0b1000000000000000000000000+1; //En caso afirmativo realizamos el complemento a 2 de P.
        compP = true; // Adicionalmente, decimos que hemos complementado P y es afirmativo.
    }

    //10. A diferencia del paso anterior, comprobamos si los signos son iguales y el acarreo es 1.
    if(signoA == signoB && C==1){
        st = g | r | st; //Sticky será igual al OR de ground, round y sticky.

        r = P%2;

        P = P>>1; //Desplazamos P 1 bit a la derecha con ">>".
        P += bitPos.at(23);
        
        expR++; //Al perder un bit significativo, sumamos 1 al exponente de la suma.
    }
    else{ //En caso de no haber cumplido el primer IF de todos, realizamos lo siguiente.
        int k = 0; //"k" corresponde al número de bits necesarios para desplazar P para que sea una mantisa normalizada (24 bits y el bit 23 sea 1)
        for(unsigned int aux = P; aux != 0 && (aux & bitPos.at(23)) == 0; aux <<=1) k++; //Recorremos P en forma de auxiliar para no modificar P internamente moviendo por cada iteración un bit a la izquierda con "<<". Además cada iteración suma 1 a k.
        if (k == 0){ //En caso de "k" ser 0, sticky será igual a un OR entre round y sticky, posteriormente round será g (bit de guarda).
            st = r|st;
            r = g;
        }
        else{
            st = 0; r = 0; //En caso contrario, sticky y round serán 0.
        }

        for(int i = 0; i < k; i++){//Posteriormente, desplazaremos P a la izquierda "k" bits.
            P<<=1;
            P += g;
        }
        expR -= k; //Establecemos el exponente de la suma a sí misma menos k.
        if(P == 0) expR = 0; //Si P es 0, el exponente de la suma será 0.
    }

    //11. Redondeamos P comprobando si round y sticky son 1 OR round es 1, sticky 0 y el bit 0 de P es 1.
    if((r==1 && st == 1)||(r==1 && st == 0 && P%2 == 1)){
        unsigned int C2 = calcularAcarreo(P, 0b100000000000000000000,0,0); //En tal caso, sacamos el acarreo de P + 1.
        P = P+1; //Además, ya que nuestro método calcularAcarreo usa binarios, sumamos posteriormente 1 para coincidir con el algoritmo.

        if(C2) { //Comrpobamos si C2 = 1.
            P >>= 1; //Desplazamos P un bit a la derecha.
            P += bitPos.at(23);
            expR++; //El exponente de la suma será sí mismo más uno.
        }
    }

    //12. Calculamos el signo del resultado comprobando si el intercambio de Operandos es 0 y si P ha sido complementada.

    signoSuma = (!opChanged && compP) ? signoB:signoA; //En caso afirmativo, el signo de la suma será el signo de B, en cambio, en caso negativo el signo de la suma será el signo de A.

    //DENORMALES
    if(expR>0b11111111){ //En caso de que el exponente sea 128 (o todo 1s), decimos que es Infinito o -Infinito dependiendo de su signo.
        return (signoSuma)? -Q_INFINITY:Q_INFINITY;
    }
    else if(expR<0){

        unsigned int t = 1 - expR;

        P >>=t;

        expR = 0;
    }
    return IEEE754Converter::IEEtofloat(signoSuma, expR, P);
}

void MainWindow::on_multiplicacion_clicked()
{
    float op1 = ui->op1Real->text().toFloat();

    float op2 = ui->op2Real->text().toFloat();

    float salida = multiplyOperation(op1,op2);

    binaryWriteIn( ui->op1IEEE, IEEE754Converter::floattoIEESign(op1), IEEE754Converter::floattoIEEExp(op1), IEEE754Converter::floattoIEEMantisa(op1));
    hexWriteIn(ui->op1Hex,  IEEE754Converter::floattoIEESign(op1), IEEE754Converter::floattoIEEExp(op1), IEEE754Converter::floattoIEEMantisa(op1));

    binaryWriteIn( ui->op2IEEE,  IEEE754Converter::floattoIEESign(op2), IEEE754Converter::floattoIEEExp(op2), IEEE754Converter::floattoIEEMantisa(op2));
    hexWriteIn(ui->op2Hex, IEEE754Converter::floattoIEESign(op2), IEEE754Converter::floattoIEEExp(op2), IEEE754Converter::floattoIEEMantisa(op2));

    ui->resulReal->setText(QString::fromStdString(std::to_string(salida)));
    binaryWriteIn(ui -> resulIEEE, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
    hexWriteIn(ui->resulHex, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
}

float MainWindow::multiplyOperation(float op1, float op2){

    unsigned int signoA = IEEE754Converter::floattoIEESign(op1);
    unsigned int signoB = IEEE754Converter::floattoIEESign(op2);

    unsigned int expA = IEEE754Converter::floattoIEEExp(op1);
    unsigned int expB = IEEE754Converter::floattoIEEExp(op2);

    unsigned int manA = IEEE754Converter::floattoIEEMantisa(op1) + bitPos.at(23);
    unsigned int manB = IEEE754Converter::floattoIEEMantisa(op2) + bitPos.at(23);

    //Paso 1: Calculamos el signo del producto
    unsigned int signoR = signoA ^ signoB;
    //Paso 2: calculamos el exponente del producto
    int expR = expA + expB - 0b1111111;

    //Paso 3: Cálculo de la mantisa del producto, mp
    //Paso 3i: Se utiliza el algoritmo del producto de enteros sin signo
    unsigned int c = 0;
    unsigned int P = 0;
    unsigned int A = manA;

    for(int i = 0; i < 24; i++){
            P+=A%2*manB;
            A = (A>>1) + (P%2)*bitPos.at(23);
            P = (P>>1) + c*bitPos.at(23);
            c>>=1;
    }

    //Paso 3ii:
    //P y A tienen 24 bits en la multiplicación
    //Si Pn-1 = 0 -> desplazar P (P, A ) un bit a la izquierda
    if((P & bitPos.at(23))==0){
        P <<= 1;
    }
    else{ //Si no se suma 1 al exponente del producto
        expR++;
    }

    //Paso 3iii: bit de redondeo -> r = An-1
    unsigned int r = (A & bitPos.at(23))!= 0;

    //Paso 3iv: Bit sticky -> st = OR(An-2, An-3, ..., A0) (Siendo n el nº de bits de la mantisa)
    unsigned int st = 0;
    for(int i = 0; i < 23; i++) st |= (A & bitPos.at(i))!= 0;

    //Paso 3v: Redondeo: Si (r=1 y st=1) O (r=1 y st=0 y P0=1) -> P = P + 1
    if((r&&st) ||(r&&!st&&P%2)) P+=1;

    //DESBORDAMIENTOS
    //Hay desbordamiento a infinito (overflow) cuando el exponente del producto es mayor que el máximo representable
    /*  ||
     *  ||
     */

    //Tratamiento de desbordamiento a cero (underflow)

    if(expR>0b11111111){
        return (signoR)? -Q_INFINITY:Q_INFINITY;
    }
    //DENORMALES
    //Si exponenteProducto < exponenteMínimo
    else if(expR<0){

        unsigned int t = 1 - expR; // t = exponenteMínimo - exponenteProducto

        //Si t >= nº bits mantisa -> hay underflow (porque se desplaza toda la mantisa)

        //Si no:
        //Desplaza aritméticamente P (P,A) t bits a la derecha
        //Nota: el resultado será un valor denormal
        P >>=t;

        //ExponenteProducto = exponenteMínimo
        expR = 0;
    }
    return IEEE754Converter::IEEtofloat(signoR, expR, P);
}

void MainWindow::on_division_clicked()
{

    float op1 = ui->op1Real->text().toFloat();

    float op2 = ui->op2Real->text().toFloat();

    float salida = divisionOperation(op1,op2);

    binaryWriteIn( ui->op1IEEE, IEEE754Converter::floattoIEESign(op1), IEEE754Converter::floattoIEEExp(op1), IEEE754Converter::floattoIEEMantisa(op1));
    hexWriteIn(ui->op1Hex,  IEEE754Converter::floattoIEESign(op1), IEEE754Converter::floattoIEEExp(op1), IEEE754Converter::floattoIEEMantisa(op1));

    binaryWriteIn( ui->op2IEEE,  IEEE754Converter::floattoIEESign(op2), IEEE754Converter::floattoIEEExp(op2), IEEE754Converter::floattoIEEMantisa(op2));
    hexWriteIn(ui->op2Hex, IEEE754Converter::floattoIEESign(op2), IEEE754Converter::floattoIEEExp(op2), IEEE754Converter::floattoIEEMantisa(op2));


    if(op2 == 0){ //Comrpobamos si el segundo operando era 0, en tal caso lo consideramos como Not a Number (NaN)
        ui -> resulIEEE -> setText("NaN");
        ui -> resulReal -> setText("NaN");
        ui -> resulHex -> setText("NaN");
    }else if(salida == INFINITY){ //Si la salida devuelve infinito, estableceremos el resultado real en infinito.
        ui -> resulReal -> setText("inf");
        binaryWriteIn(ui -> resulIEEE, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
        hexWriteIn(ui->resulHex, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
    }else if(salida == -INFINITY){ //Lo mismo que el paso anterior, pero en caso de recibir -Infinito.
        ui -> resulReal -> setText("-inf");
        binaryWriteIn(ui -> resulIEEE, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
        hexWriteIn(ui->resulHex, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
    }else{
        ui->resulReal->setText(QString::fromStdString(std::to_string(salida)));
        binaryWriteIn(ui -> resulIEEE, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
        hexWriteIn(ui->resulHex, IEEE754Converter::floattoIEESign(salida), IEEE754Converter::floattoIEEExp(salida), IEEE754Converter::floattoIEEMantisa(salida));
    }
}

float MainWindow::divisionOperation(float op1, float op2)
{
    if(IEEE754Converter::floattoIEEExp(op1) > 254 && IEEE754Converter::floattoIEEExp(op2)){ //Si ambos exponentes son mayores que 254, diremos que dará infinito.
        return INFINITY;
    }

    if(op1 == op2){ //Para evitar recorrer el programa innecesariamente, comprobamos si ambos operandos son iguales para devolver que la división es igual a 1.
        return 1;
    }

    unsigned int signoA = IEEE754Converter::floattoIEESign(op1);
    unsigned int signoB = IEEE754Converter::floattoIEESign(op2);

    unsigned int expA = IEEE754Converter::floattoIEEExp(op1);
    unsigned int expB = IEEE754Converter::floattoIEEExp(op2);

    unsigned int manA = IEEE754Converter::floattoIEEMantisa(op1) + bitPos.at(23);
    unsigned int manB = IEEE754Converter::floattoIEEMantisa(op2) + bitPos.at(23);
 
    //Paso 1: Hacemos el escalado de mantissas, multiplicando desde 0 hasta -infinito (1*2^0 + 1*2^-1 + 1*2^-2...) toda la mantisa.
    QString mantisaA = toMantisa(manA);
    QString mantisaB = toMantisa(manB);

    float escaladoA = 0, escaladoB = 0;

    for(int i = 0; i < 24; i++){
        if(mantisaA.at(i).digitValue() == 1) {
            escaladoA += 1 * pow(2, -i);
        }
        if(mantisaB.at(i).digitValue() == 1) {
            escaladoB += 1 * pow(2, -i);
        }
    }

    //Paso 2: Comprobamos si escaladoB está en una de estas dos franjas: [1 , 1.25) = 1 y [1.25 , 2) = 0.8.

    float Bprima = 0;

    if(1 >= escaladoB && escaladoB < 1.25){
        Bprima = 1;
    }else{
        Bprima = 0.8;
    }

    //Paso 3: Obtenemos Xo e Yo. Lo hacemos simplemente multiplicando su escalado (X es A, Y es B) por Bprima.

    float Xo = multiplyOperation(escaladoA, Bprima);
    float Yo = multiplyOperation(escaladoB, Bprima);
    float r = addOperation(2 , -Yo);

    float Ax1B = 0; //A x el inverso de B'

    //Paso 4: Iteramos estos valores hasta que la resta de Xi - Xi-1 sea menor que 10 ^ -4 (0.0001).

    bool cond = true;

    while(cond){ //Establecemos una condición permanentemente verdadera hasta que Xnueva - Xo sea > 0.0001 o de 0.
        float Xnueva = multiplyOperation(Xo, r); //La X nueva de cada iteración.
        float Ynueva = multiplyOperation(Yo, r); //La Y nueva de cada iteración.

        if((addOperation(Xnueva, -Xo)) > 0.0001 || addOperation(Xnueva, -Xo) == 0){
            cond = false;
            Ax1B = Xnueva;
        }

        Xo = Xnueva; //En caso de no haberse cumplido la condición, Xnueva será el nuevo valor de Xo, el cual será usado para la resta con la siguiente Xnueva.

        r = addOperation(2, -Ynueva); //r, la cual es usada para calcular X/Y nueva, se obtiene restando 2 - Ynueva.

        Yo = Ynueva;
    }

    //Paso 5: Calculamos con un XOR el valor del signo de la división.

    unsigned int divSigno = signoA != signoB;

    //Paso 6 Obtenemos el exponente de la división restando exponente de A - exponente de B y sumándole el exponente de Bprima(Ax1B):

    unsigned int divExp = expA - expB + IEEE754Converter::floattoIEEExp(Ax1B); //Realizamos las operaciones normales ya que nuestro método de la ALU opera con floats y los valores dados son ints.

    if(divExp > 254 && divSigno == 0) //Comprobamos de antemano si el exponente es mayor que 254 y su signo para determinar si es infinito o -infinito.
    {
        return INFINITY;
    }else if(divExp > 254 && divSigno == 1)
    {
        return -INFINITY;
    }

    //Paso 7

    unsigned int divMantisa = IEEE754Converter::floattoIEEMantisa(Ax1B); //La mantisa de la división será la mantisa de Bprima.

    return IEEE754Converter::IEEtofloat(divSigno, divExp, divMantisa);
}

QString MainWindow::toMantisa(unsigned int mantisa)
{
    QString binaryNumber;

    for(int i =0;i< 23;i++){
        binaryNumber.push_front(QString::fromStdString(std::to_string(mantisa%2)));
        mantisa/=2;
    }

    binaryNumber.push_front("1");

    return binaryNumber;
}

float MainWindow::denormalCalculator(unsigned int sign, unsigned int mantissa){
    float preMantissa =1-IEEE754Converter::IEEtofloat(0,127,mantissa);
    if(sign){
        return mantissa * 1.1754944e-38;//2^-126 para la alu
    }else{
        return mantissa * -1.1754944e-38;//-2^-126 para la alu
    }
}

void MainWindow::binaryWriteIn(QLineEdit* child, unsigned int sign, unsigned int exp, unsigned int mantisa)
{
    QString binaryNumber;

    for(int i =0;i< 23;i++){
        binaryNumber.push_front(QString::fromStdString(std::to_string(mantisa%2)));
        mantisa/=2;
    }

    for(int i=0;i<8;i++){
        binaryNumber.push_front(QString::fromStdString(std::to_string(exp%2)));
        exp /= 2;
    }

    binaryNumber.push_front(QString::fromStdString(std::to_string(sign)));

    binaryNumber.push_front("0b");

    child->setText(binaryNumber);
}

unsigned int MainWindow::calcularAcarreo(unsigned int manA, unsigned int manB, unsigned int pos, unsigned int acarreoActual)
{
    if(pos == 24) return acarreoActual;

    if((manB & bitPos.at(pos)) != 0 && (manA & bitPos.at(pos)) != 0) return calcularAcarreo(manA, manB, pos+1, 1);
    else if(((manB & bitPos.at(pos)) != 0 || (manA & bitPos.at(pos)) != 0) && acarreoActual != 0) return calcularAcarreo(manA, manB, pos+1, 1);
    else return calcularAcarreo(manA, manB, pos+1, 0);
}

void MainWindow::on_Reset_clicked()
{
    ui->op1Real->setText("");
    ui->op2Real->setText("");
    ui->op1IEEE->setText("");
    ui->op2IEEE->setText("");
    ui->op1Hex->setText("");
    ui->op2Hex->setText("");
    ui->resulReal->setText("");
    ui->resulIEEE->setText("");
    ui->resulHex->setText("");
}

void MainWindow::hexWriteIn(QLineEdit* child, unsigned int sign, unsigned int exp, unsigned int mantisa){

    QString stringHex;

    unsigned int aux = (sign << 31) + (exp << 23) + (mantisa);

    for(int i = 0; i < 8; i++) {
        unsigned int mod = aux%16;
        switch (mod) 
        {
        case 10: stringHex.push_front('A');break;
        case 11: stringHex.push_front('B');break;
        case 12: stringHex.push_front('C');break;
        case 13: stringHex.push_front('D');break;
        case 14: stringHex.push_front('E');break;
        case 15: stringHex.push_front('F');break;
        default: stringHex.push_front(QString::fromStdString(std::to_string(mod)));
        }
        aux >>= 4;
    }
    child->setText("0x"+stringHex);
}
