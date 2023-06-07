#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <ieee754converter.h>

#include <iostream>

//Aquí hacemos las funciones de la ALU
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->num1IEEE->setReadOnly(true);
    ui->num1Hex->setReadOnly(true);
    ui->num2IEEE->setReadOnly(true);
    ui->num2Hex->setReadOnly(true);
    ui->resulReal->setReadOnly(true);
    ui->resulIEEE->setReadOnly(true);
    ui->resulHex->setReadOnly(true);

    bits.push_back(1);

    for(int i = 1; i < 32; i++) {
        bits.push_back(2*bits.at(i-1));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::getMethod(int clicked, float num1, float num2)
{
    switch(clicked)
    {
    case 1:
    {
        float addResult = addOperation(num1, num2);

        QString qRes;

        if (addResult >= 1e7 || addResult <= -10000000000){
            qRes = QString::number(addResult, 'E', 0);

            if (addResult >= 1e7) {
                qRes.replace("+", "");
            }
        }
        else {
            qRes = QString::number(addResult, 'f');
        }

        print(addResult, num1, num2, qRes);
        break;
    }

    case 2:
    {
        float multResult = multOperation(num1, num2);

        QString qRes;

        if (multResult >= 1e7 || multResult <= -10000000000){
            qRes = QString::number(multResult, 'E', 0);

            if (multResult >= 1e7) {
                qRes.replace("+", "");
            }
        }
        else {
            qRes = QString::number(multResult, 'f');
        }

        print(multResult, num1, num2, qRes);
        break;
    }
    }
}

void MainWindow::on_add_clicked()
{
    float num1 = ui->num1Real->text().toFloat();

    float num2 = ui->num2Real->text().toFloat();

    getMethod(1, num1, num2);
}

float MainWindow::addOperation(float num1, float num2)
{
    //Pasos previos

    unsigned long aSign = IEEE754Converter::floattoIEESign(num1);
    unsigned long aExpo = IEEE754Converter::floattoIEEExp(num1);
    unsigned long aMant = IEEE754Converter::floattoIEEMantisa(num1) + bits.at(23);

    unsigned long bSign = IEEE754Converter::floattoIEESign(num2);
    unsigned long bExpo = IEEE754Converter::floattoIEEExp(num2);
    unsigned long bMant = IEEE754Converter::floattoIEEMantisa(num2) + bits.at(23);
    
    //1. Inicializamos los bits de guarda, sticky y round a 0. Además, inicializamos operandos intercambiados y complementado de P a falso
    unsigned long addSign;

    unsigned long g = 0, r = 0, st = 0;
    bool change = false, complement_P = false;

    //2. Coomprobamos si el exponente de A es menor que el de B, en tal caso, se cambian de valores, pasando num2 a ser el num1, y el num1 a ser el num2.
    if(bExpo > aExpo)
    {
        unsigned long aSignux = aSign, aExpoux = aExpo, aMantux = aMant;
        aMant = bMant, aExpo = bExpo, aSign = bSign;
        bMant = aMantux, bExpo = aExpoux, bSign = aSignux;
        change = true;
    }
    
    //3. El exponente de la add se convierte ne el exponente de A y "d" es la resta entre ambos exponentes (aExpo - bExpo).
    unsigned long d = aExpo - bExpo;
    int expR = aExpo;
    
    //4. Comprobamos si los signos son diferentes, en tal caso, la mantisa de B se convierte en su complemento a 2.
    if(aSign!=bSign)
    {
        bMant = getC2(bMant);
    }

    //5. Creamos P igualándolo a la mantisa de B.
    unsigned long P = bMant;
    
    //6. Asignamos los bits de guarda, sticky y round solamente si d es mayor o igual a 3 y estrictamente menor que 25.
    //Ej: Si d = 7 y P = 010110000000000000000000 -> g = 0 (d-1=6, contamos 6 bits desde la DERECHA); el bit r la posición d-2, sticky OR de cada bit desde el bit d-3 hasta el bit 0
    if (d >= 3 && d < 25)
    {
        g = (bits[d-1] & P) != 0;
        r = (bits[d-2] & P) != 0;
        st = (bits[d-3] & P) != 0;

        for (int i = d-3; i > 0 && !st; i--)
        {
            st |= (bits[d-i] & P) != 0; //El bit sticky realiza un OR para establecer su valor.
        }
    }
    
    //7. Comprobamos de nuevo si los signos son diferentes. En tal caso, desplazamos P a la derecha "d" bits introduciendo unos por la izquierda.
    if(aSign!=bSign)
    {
        for(unsigned long i = 0; i < d; i++)
        {
            P >>= 1;
            P += bits.at(23);
        }
    }else
    {
        P >>= d; //En caso de ser iguales desplazamos P a la derecha "d" bits introduciendo ceros por la izquierda.
    }
    //8. Hacemos que P sea si misma más mantisa de A. El acarreo es el resultado de la add entre ambas en caso de que haya habido un bit "1" adicional al principio.
    unsigned long C = carry(aMant, P, 0, 0);
    P += aMant;

    //9. Comprobamos si los signos son diferentes, si el bit 24 - 1 es 1 y si el acarreo es 0.
    bool cond1 = (aSign != bSign);
    bool cond2 = ((P & bits.at(23)) != 0 && C == 0);

    if(cond1 && cond2){
        P = getC2(P); //En caso afirmativo realizamos el complemento a 2 de P.
        complement_P = true; // Adicionalmente, decimos que hemos complementado P y es afirmativo.
    }

    //10. A diferencia del paso anterior, comprobamos si los signos son iguales y el acarreo es 1.
    if(!cond1 && C==1){
        st |= g | r; //Sticky será igual al OR de ground, round y sticky.

        r = P % 2;

        P >>= 1; //Desplazamos P 1 bit a la derecha con ">>".
        P += bits.at(23);
        
        expR++; //Al perder un bit significativo, addmos 1 al exponente de la add.
    }
    else{ //En caso de no haber cumplido el primer IF de todos, realizamos lo siguiente.
        int k = 0; //"k" corresponde al número de bits necesarios para desplazar P para que sea una mantisa normalizada (24 bits y el bit 23 sea 1)
        for(unsigned long aux = P; aux != 0 && (aux & bits.at(23)) == 0; aux <<=1) k++; //Recorremos P en forma de auxiliar para no modificar P internamente moviendo por cada iteración un bit a la izquierda con "<<". Además cada iteración add 1 a k.
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
        expR -= k; //Establecemos el exponente de la add a sí misma menos k.
        if(P == 0) expR = 0; //Si P es 0, el exponente de la add será 0.
    }

    //11. Redondeamos P comprobando si round y sticky son 1 OR round es 1, sticky 0 y el bit 0 de P es 1.
    bool condition1 = (r == 1 && st == 1);
    bool condition2 = (r == 1 && st == 0 && P % 2 == 1);

    if ((condition1 || condition2) && carry(P, 0b100000000000000000000, 0, 0)) {
        P = ((P + 1) >> 1) + bits.at(23);
        expR++;
    }

    //12. Calculamos el signo del resultado comprobando si el intercambio de Operandos es 0 y si P ha sido complementada.

    if (!change && complement_P) {
        addSign = bSign; //En caso afirmativo, el signo de la add será el signo de B, en cambio, en caso negativo el signo de la add será el signo de A.
    } else {
        addSign = aSign;
    }

    //DENORMALES
    if(expR>0b11111111){ //En caso de que el exponente sea 128 (o todo 1s), decimos que es Infinito o -Infinito dependiendo de su signo.
        return (addSign)? -Q_INFINITY:Q_INFINITY;
    }
    else if(expR<0){

        unsigned long t = 1 - expR;

        P >>=t;

        expR = 0;
    }
    return IEEE754Converter::IEEtofloat(addSign, expR, P);
}

void MainWindow::on_mult_clicked()
{
    float num1 = ui->num1Real->text().toFloat();

    float num2 = ui->num2Real->text().toFloat();

    getMethod(2, num1, num2);
}

float MainWindow::multOperation(float num1, float num2){

    unsigned long signo1 = IEEE754Converter::floattoIEESign(num1);
    unsigned long signo2 = IEEE754Converter::floattoIEESign(num2);

    unsigned long exp1 = IEEE754Converter::floattoIEEExp(num1);
    unsigned long exp2 = IEEE754Converter::floattoIEEExp(num2);

    unsigned long man1 = IEEE754Converter::floattoIEEMantisa(num1) + bits.at(23);
    unsigned long man2 = IEEE754Converter::floattoIEEMantisa(num2) + bits.at(23);

    /*              RESULTADOS              */
    //Cuando el resultado es "NaN" al realizar los cálculos da 0,0000000

    /*Paso 1: Calculamos el signo del producto*/
    unsigned long signoR = signo1 ^ signo2;

    /*Paso 2: calculamos el exponente del producto*/
    int expR = exp1 + exp2 - 0b1111111;

    /*Paso 3: Cálculo de la mantisa del producto, mp*/
    /*Paso 3i: Se utiliza el algoritmo del producto de enteros sin signo*/
    unsigned long c = 0;
    unsigned long P = 0;
    unsigned long A = man1;

    for (int i = 0; i < 24; i++) {
        unsigned long bitMenosSignif = A & 1; // Obtener el bit menos significativo de A

        if (bitMenosSignif != 0) {
            P = P + man2; // addr man2 a P si el bit menos significativo de A es 1
        }

        A = A >> 1; // Desplazar A hacia la derecha 1 bit

        c = P & (1 << 24); // Obtener el bit de carry

        P = P >> 1; // Desplazar P hacia la derecha 1 bit

        if (c != 0) {
            P = P | (1 << 23); // Establecer el bit más significativo de P si hubo carry
        }
    }

    /*Paso 3ii:*/
    //P y A tienen 24 bits en la multiplicación
    //Si Pn-1 = 0 -> desplazar P (P, A ) un bit a la izquierda
    if ((P & bits.at(23)) == 0) {
        P = P << 1;
    }
    else { //Si no se add 1 al exponente del producto
        expR++;
    }

    /*Paso 3iii: bit de redondeo -> r = An-1*/
    //Operación bit a bit AND entre A y el bit 23
    unsigned long bitAbitOpAND = A & bits.at(23);
    unsigned long r;

    if (bitAbitOpAND != 0) {
        r = 1;
    } else {
        r = 0;
    }

    /*Paso 3iv: Bit sticky -> st = OR(An-2, An-3, ..., A0) (Siendo n el nº de bits de la mantisa)*/
    unsigned long st = 0;
    unsigned long bitAbitOpOR = 0;

    for(int i = 0; i < 23; i++) {
        bitAbitOpOR = A & bits.at(i);

        if (bitAbitOpOR != 0) { //En cuanto un bit es distinto de cero -> sticky = 1 y se sale del bucle
            st = 1;
            break;
        }
    }

    /*Paso 3v.i: Redondeo: Si (r=1 y st=1) O (r=1 y st=0 y P0=1) -> P = P + 1*/
    if ((r == 1 && st == 1) || (r == 1 && st == 0 && (P % 2 != 0))) {
        P = P + 1;
    }

    //3v.ii. Comprobación de si se produce desbordamiento
    int desbordamiento = calculateOverflow(expR);

    if (desbordamiento == 0) {  //overflow
        return Q_INFINITY;
    }
    else if (desbordamiento == 1) { //underflow
        unsigned long t = 0 - expR; // t = exponenteMínimo - exponenteProducto

        //Desplaza aritméticamente P (P,A) t bits a la derecha
        //Nota: el resultado será un valor denormal
        P = P >> t;

        //ExponenteProducto = exponenteMínimo
        expR = 0;
    }

    //3v.iiii. manP = P;
    unsigned long manP = P;

    return IEEE754Converter::IEEtofloat(signoR ,expR, manP);
}

void MainWindow::on_div_clicked() {

    float num1 = ui->num1Real->text().toFloat();

    float num2 = ui->num2Real->text().toFloat();

    float result = divOperation(num1,num2);

    QString num1IEEE = toIEEEString(IEEE754Converter::floattoIEESign(num1), IEEE754Converter::floattoIEEExp(num1), IEEE754Converter::floattoIEEMantisa(num1));
    ui->num1IEEE->setText(num1IEEE);

    QString num2IEEE = toIEEEString(IEEE754Converter::floattoIEESign(num2), IEEE754Converter::floattoIEEExp(num2), IEEE754Converter::floattoIEEMantisa(num2));
    ui->num2IEEE->setText(num2IEEE);


    QString num1Hex = toHexadecimalString(IEEE754Converter::floattoIEESign(num1), IEEE754Converter::floattoIEEExp(num1), IEEE754Converter::floattoIEEMantisa(num1));
    ui->num1Hex->setText(num1Hex);

    QString num2Hex = toHexadecimalString(IEEE754Converter::floattoIEESign(num2), IEEE754Converter::floattoIEEExp(num2), IEEE754Converter::floattoIEEMantisa(num2));
    ui->num2Hex->setText(num2Hex);

    if (num2 == 0) { //Comrpobamos si el segundo operando era 0, en tal caso lo consideramos como Not a Number (NaN)
        ui -> resulIEEE -> setText("NaN");
        ui -> resulReal -> setText("NaN");
        ui -> resulHex -> setText("NaN");
    }
    else if (result == INFINITY) { //Si la result devuelve infinito, estableceremos el resultado real en infinito.
        ui -> resulReal -> setText("inf");
        QString resulIEEE = toIEEEString(IEEE754Converter::floattoIEESign(result), IEEE754Converter::floattoIEEExp(result), IEEE754Converter::floattoIEEMantisa(result));
        ui->resulIEEE->setText(resulIEEE);
        QString resulHexadecimal = toHexadecimalString(IEEE754Converter::floattoIEESign(result), IEEE754Converter::floattoIEEExp(result), IEEE754Converter::floattoIEEMantisa(result));
        ui->resulHex->setText(resulHexadecimal);
    }
    else if (result == -INFINITY) { //Lo mismo que el paso anterior, pero en caso de recibir -Infinito.
        ui -> resulReal -> setText("-inf");
        QString resulIEEE = toIEEEString(IEEE754Converter::floattoIEESign(result), IEEE754Converter::floattoIEEExp(result), IEEE754Converter::floattoIEEMantisa(result));
        ui->resulIEEE->setText(resulIEEE);
        QString resulHexadecimal = toHexadecimalString(IEEE754Converter::floattoIEESign(result), IEEE754Converter::floattoIEEExp(result), IEEE754Converter::floattoIEEMantisa(result));
        ui->resulHex->setText(resulHexadecimal);
    }
    else {
        ui->resulReal->setText(QString::fromStdString(std::to_string(result)));
        QString resulIEEE = toIEEEString(IEEE754Converter::floattoIEESign(result), IEEE754Converter::floattoIEEExp(result), IEEE754Converter::floattoIEEMantisa(result));
        ui->resulIEEE->setText(resulIEEE);
        QString resulHexadecimal = toHexadecimalString(IEEE754Converter::floattoIEESign(result), IEEE754Converter::floattoIEEExp(result), IEEE754Converter::floattoIEEMantisa(result));
        ui->resulHex->setText(resulHexadecimal);
    }
}

float MainWindow::divOperation(float num1, float num2)
{
    if(IEEE754Converter::floattoIEEExp(num1) > 254 && IEEE754Converter::floattoIEEExp(num2)){ //Si ambos exponentes son mayores que 254, diremos que dará infinito.
        return INFINITY;
    }

    if(num1 == num2){ //Para evitar recorrer el programa innecesariamente, comprobamos si ambos operandos son iguales para devolver que la división es igual a 1.
        return 1;
    }

    unsigned long aSign = IEEE754Converter::floattoIEESign(num1);
    unsigned long bSign = IEEE754Converter::floattoIEESign(num2);

    unsigned long aExpo = IEEE754Converter::floattoIEEExp(num1);
    unsigned long bExpo = IEEE754Converter::floattoIEEExp(num2);

    unsigned long aMant = IEEE754Converter::floattoIEEMantisa(num1) + bits.at(23);
    unsigned long bMant = IEEE754Converter::floattoIEEMantisa(num2) + bits.at(23);
 
    //Paso 1: Hacemos el escalado de mantissas, multiplicando desde 0 hasta -infinito (1*2^0 + 1*2^-1 + 1*2^-2...) toda la mantisa.
    QString mantisaA = toMantisa(aMant);
    QString mantisaB = toMantisa(bMant);

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

    float Xo = multOperation(escaladoA, Bprima);
    float Yo = multOperation(escaladoB, Bprima);
    float r = addOperation(2 , -Yo);

    float Ax1B = 0; //A x el inverso de B'

    //Paso 4: Iteramos estos valores hasta que la resta de Xi - Xi-1 sea menor que 10 ^ -4 (0.0001).

    bool cond = true;

    while(cond){ //Establecemos una condición permanentemente verdadera hasta que Xnueva - Xo sea > 0.0001 o de 0.
        float Xnueva = multOperation(Xo, r); //La X nueva de cada iteración.
        float Ynueva = multOperation(Yo, r); //La Y nueva de cada iteración.

        if((addOperation(Xnueva, -Xo)) > 0.0001 || addOperation(Xnueva, -Xo) == 0){
            cond = false;
            Ax1B = Xnueva;
        }

        Xo = Xnueva; //En caso de no haberse cumplido la condición, Xnueva será el nuevo valor de Xo, el cual será usado para la resta con la siguiente Xnueva.

        r = addOperation(2, -Ynueva); //r, la cual es usada para calcular X/Y nueva, se obtiene restando 2 - Ynueva.

        Yo = Ynueva;
    }

    //Paso 5: Calculamos con un XOR el valor del signo de la división.

    unsigned long divSigno = aSign != bSign;

    //Paso 6 Obtenemos el exponente de la división restando exponente de A - exponente de B y sumándole el exponente de Bprima(Ax1B):

    unsigned long divExp = aExpo - bExpo + IEEE754Converter::floattoIEEExp(Ax1B); //Realizamos las operaciones normales ya que nuestro método de la ALU opera con floats y los valores dados son ints.

    if(divExp > 254 && divSigno == 0) //Comprobamos de antemano si el exponente es mayor que 254 y su signo para determinar si es infinito o -infinito.
    {
        return INFINITY;
    }else if(divExp > 254 && divSigno == 1)
    {
        return -INFINITY;
    }

    //Paso 7

    unsigned long divMantisa = IEEE754Converter::floattoIEEMantisa(Ax1B); //La mantisa de la división será la mantisa de Bprima.

    return IEEE754Converter::IEEtofloat(divSigno, divExp, divMantisa);
}

unsigned long MainWindow::getC2(unsigned long number)
{
    return ((~number) % 0b1000000000000000000000000) + 1;
}

int MainWindow::calculateOverflow(int expResul) {

    /*Para calcular el resultado, primero se comprueba si se produce desbordamiento*/

    //Desbordamiento a infinito (overflow) cuando el exponenteResultado > expMaximoRepresentable
    if (expResul > 0b11111111){
        return 0;
    }
    //Desbordamiento a 0 (underflow)
    else if (expResul < 0){
        return 1;
    }
    else {
        //No se produce desbordamiento -> no se hace nada
        return -1;
    }
}

QString MainWindow::toMantisa(unsigned long mantisa)
{
    QString binaryNumber;

    for(int i =0;i< 23;i++){
        binaryNumber.push_front(QString::fromStdString(std::to_string(mantisa % 2)));
        mantisa = mantisa / 2;
    }

    binaryNumber.push_front("1");

    return binaryNumber;
}

QString MainWindow::toIEEEString(unsigned long signo, unsigned long exponente, unsigned long mantisa) {
    QString IEEEnumber;

    for(int i =0;i< 23;i++){
        IEEEnumber.push_front(QString::fromStdString(std::to_string(mantisa % 2)));
        mantisa = mantisa / 2;
    }

    for(int i=0;i<8;i++){
        IEEEnumber.push_front(QString::fromStdString(std::to_string(exponente % 2)));
        exponente = exponente / 2;
    }

    IEEEnumber.push_front(QString::fromStdString(std::to_string(signo)));

    return IEEEnumber;
}

unsigned long MainWindow::carry(unsigned long aMant, unsigned long bMant, unsigned long pos, unsigned long actualCarry)
{
    if (pos == 24) {
        return actualCarry;
    }

    bool bitA = (aMant & bits.at(pos)) != 0;
    bool bitB = (bMant & bits.at(pos)) != 0;

    unsigned long newCarry = (bitA && bitB) || (actualCarry && (bitA || bitB));

    return carry(aMant, bMant, pos + 1, newCarry);
}



QString MainWindow::toHexadecimalString(unsigned long signo, unsigned long exponente, unsigned long mantisa) {

    QString hexadecimalNumber;

    unsigned long aux = (signo << 31) + (exponente << 23) + (mantisa);

    for (int i = 0; i < 8; i++) {
        unsigned long mod = aux % 16;

        switch (mod) {
            case 10: hexadecimalNumber.push_front('A');
                break;
            case 11: hexadecimalNumber.push_front('B');
                break;
            case 12: hexadecimalNumber.push_front('C');
                break;
            case 13: hexadecimalNumber.push_front('D');
                break;
            case 14: hexadecimalNumber.push_front('E');
                break;
            case 15: hexadecimalNumber.push_front('F');
                break;

            default: hexadecimalNumber.push_front(QString::fromStdString(std::to_string(mod)));
        }

        aux = aux >> 4;
    }

    hexadecimalNumber.push_front("0x");

    return hexadecimalNumber;
}

void MainWindow::print(float result, float num1, float num2, QString result2)
{
    QString num1IEEE = toIEEEString(IEEE754Converter::floattoIEESign(num1), IEEE754Converter::floattoIEEExp(num1), IEEE754Converter::floattoIEEMantisa(num1));
    ui->num1IEEE->setText(num1IEEE);

    QString num2IEEE = toIEEEString(IEEE754Converter::floattoIEESign(num2), IEEE754Converter::floattoIEEExp(num2), IEEE754Converter::floattoIEEMantisa(num2));
    ui->num2IEEE->setText(num2IEEE);


    QString num1Hex = toHexadecimalString(IEEE754Converter::floattoIEESign(num1), IEEE754Converter::floattoIEEExp(num1), IEEE754Converter::floattoIEEMantisa(num1));
    ui->num1Hex->setText(num1Hex);

    QString num2Hex = toHexadecimalString(IEEE754Converter::floattoIEESign(num2), IEEE754Converter::floattoIEEExp(num2), IEEE754Converter::floattoIEEMantisa(num2));
    ui->num2Hex->setText(num2Hex);

    ui->resulReal->setText(result2);

    QString resulIEEE = toIEEEString(IEEE754Converter::floattoIEESign(result), IEEE754Converter::floattoIEEExp(result), IEEE754Converter::floattoIEEMantisa(result));
    ui->resulIEEE->setText(resulIEEE);

    QString resulHex = toHexadecimalString(IEEE754Converter::floattoIEESign(result), IEEE754Converter::floattoIEEExp(result), IEEE754Converter::floattoIEEMantisa(result));
    ui->resulHex->setText(resulHex);
}

void MainWindow::on_Reset_clicked() {
    ui->num1Real->setText("");
    ui->num2Real->setText("");
    ui->num1IEEE->setText("");
    ui->num2IEEE->setText("");
    ui->num1Hex->setText("");
    ui->num2Hex->setText("");
    ui->resulReal->setText("");
    ui->resulIEEE->setText("");
    ui->resulHex->setText("");
}
