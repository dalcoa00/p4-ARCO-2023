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

    binaryWriteIn( ui->op1IEEE, IEEE754Converter::floattoIEESign(op1), IEEE754Converter::floattoIEEExp(op1), IEEE754Converter::floattoIEEMantisa(op1));
    hexWriteIn(ui->op1Hex,  IEEE754Converter::floattoIEESign(op1), IEEE754Converter::floattoIEEExp(op1), IEEE754Converter::floattoIEEMantisa(op1));

    binaryWriteIn( ui->op2IEEE,  IEEE754Converter::floattoIEESign(op2), IEEE754Converter::floattoIEEExp(op2), IEEE754Converter::floattoIEEMantisa(op2));
    hexWriteIn(ui->op2Hex, IEEE754Converter::floattoIEESign(op2), IEEE754Converter::floattoIEEExp(op2), IEEE754Converter::floattoIEEMantisa(op2));

    ui->resulReal->setText(QString::fromStdString(std::to_string(salida)));
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


    const unsigned int excsBits = bitPos.at(31)+bitPos.at(30)+bitPos.at(29)+bitPos.at(28)+bitPos.at(27)+bitPos.at(26)+bitPos.at(25)+bitPos.at(24);

    //1. Inicializar guarda, sticky, round, operandos y complementado.
    unsigned int signoSuma;

    unsigned int g = 0; unsigned int r = 0; unsigned int st = 0;
    bool opChanged = false;
    bool compP = false;

    //2. Comprobar exponentes.
    if(expA < expB){
        expA = IEEE754Converter::floattoIEEExp(op2);
        expB = IEEE754Converter::floattoIEEExp(op1);
        manA = IEEE754Converter::floattoIEEMantisa(op2) + bitPos.at(23);
        manB = IEEE754Converter::floattoIEEMantisa(op1) + bitPos.at(23);
        signoA = IEEE754Converter::floattoIEESign(op2);
        signoB = IEEE754Converter::floattoIEESign(op1);
        opChanged = true;
    }
    //3. Calculamos exponente de la suma y de "d".
    int expR = expA;
    unsigned int d = expA - expB;
    //4. Comprobamos si los signos son diferentes.

    if(signoA!=signoB) manB = (~manB)%0b1000000000000000000000000+1;

    //5. Creamos P.
    unsigned int P = manB;
    //6. Establecemos los bits de guarda, sticky y round.
    if(d>=3 && d < 25){
        g = (bitPos.at(d-1)&P)!=0;
        r = (bitPos.at(d-2)&P)!=0;
        st = (bitPos.at(d-3)&P)!=0;
    }
    for(int i = d-3; i > 0 && !st; i--) st = st|(bitPos.at(d-i)&P);
    //7. Comprobamos de nuevo los signos.
    if(signoA!=signoB){
        for(unsigned int i = 0; i < d; i++){
            P >>= 1;
            P += bitPos.at(23);
        }
    }
    else P = P>>d;
    //8. Sacamos el acarreo.
    unsigned int C = 0;
    C = calcularAcarreo(manA, P, 0, 0);
    P = manA + P;

    //9. Comprobamos el signo, la P y el acarreo.
    if(signoA!=signoB && (P & bitPos.at(23)) != 0 && C == 0){
        P = (~P)%0b1000000000000000000000000+1;
        compP = true;
    }

    //10. Asimismo, comprobamos si el signo es el mismo y el acarreo es 1.
    if(signoA == signoB && C==1){

        st = g | r | st;

        r = P%2;

        P = P>>1;
        P += bitPos.at(23);

        expR++;

    }
    else{

        int k = 0;
        for(unsigned int aux = P; aux != 0 && (aux & bitPos.at(23)) == 0; aux <<=1) k++;
        if (k == 0){
            st = r|st;
            r = g;
        }
        else{
            st = 0; r = 0;
        }

        for(int i = 0; i < k; i++){

            P<<=1;
            P += g;

        }

        expR -= k;

        if(P == 0) expR = 0;

    }

    //11. Redondeamos P.
    if((r==1 && st == 1)||(r==1 && st == 0 && P%2 == 1)){
        unsigned int C2 = calcularAcarreo(P, 0b100000000000000000000,0,0);
        P = P+1;

        if(C2) {
            P >>= 1;
            P += bitPos.at(23);
            expR++;
        }
    }

    //12. Calculamos el signo del resultado

    signoSuma = (!opChanged && compP) ? signoB:signoA;

    //DENORMALES
    if(expR>0b11111111){
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

    //Paso 1:
    unsigned int signoR = signoA ^ signoB;
    //Paso 2:
    int expR = expA + expB - 0b1111111;

    //Paso 3:
    //Paso 3i:
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

    if((P & bitPos.at(23))==0){
        P <<= 1;
    }
    else{
        expR++;
    }

    //Paso 3iii:
    unsigned int r = (A & bitPos.at(23))!= 0;

    //Paso 3iv:
    unsigned int st = 0;
    for(int i = 0; i < 23; i++) st |= (A & bitPos.at(i))!= 0;

    //Paso 3v:
    if((r&&st) ||(r&&!st&&P%2)) P+=1;

    //DESBORDAMIENTOS
    if(expR>0b11111111){
        return (signoR)? -Q_INFINITY:Q_INFINITY;
    }
    else if(expR<0){

        unsigned int t = 1 - expR;

        P >>=t;

        expR = 0;

    }

    return IEEE754Converter::IEEtofloat(signoR, expR, P);

}

void MainWindow::on_division_clicked()
{
    float op1 = ui->op1Real->text().toFloat();
    float op2 = ui->op2Real->text().toFloat();

    //Pasos previos

    unsigned int signoA = IEEE754Converter::floattoIEESign(op1);
    unsigned int signoB = IEEE754Converter::floattoIEESign(op2);

    unsigned int expA = IEEE754Converter::floattoIEEExp(op1);
    unsigned int expB = IEEE754Converter::floattoIEEExp(op2);

    unsigned int manA = IEEE754Converter::floattoIEEMantisa(op1);// + bitPos.at(23);
    unsigned int manB = IEEE754Converter::floattoIEEMantisa(op2);// + bitPos.at(23);






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

        switch (mod) {

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

