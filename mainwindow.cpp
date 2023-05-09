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

void MainWindow::on_suma_clicked()
{

    float n1 = ui->op1Real->text().toFloat();

    float n2 = ui->op2Real->text().toFloat();

    float result = addOperation(n1, n2);

    binaryWriteIn( ui->op1IEEE, ConversorIEEE754::floattoIEESign(n1), ConversorIEEE754::floattoIEEExp(n1), ConversorIEEE754::floattoIEEMantissa(n1));
    hexWriteIn(ui->op1Hex,  ConversorIEEE754::floattoIEESign(n1), ConversorIEEE754::floattoIEEExp(n1), ConversorIEEE754::floattoIEEMantissa(n1));

    binaryWriteIn( ui->op2IEEE,  ConversorIEEE754::floattoIEESign(n2), ConversorIEEE754::floattoIEEExp(n2), ConversorIEEE754::floattoIEEMantissa(n2));
    hexWriteIn(ui->op2Hex, ConversorIEEE754::floattoIEESign(n2), ConversorIEEE754::floattoIEEExp(n2), ConversorIEEE754::floattoIEEMantissa(n2));

    ui->resulReal->setText(QString::fromStdString(std::to_string(result)));
    binaryWriteIn(ui -> resulIEEE, ConversorIEEE754::floattoIEESign(result), ConversorIEEE754::floattoIEEExp(result), ConversorIEEE754::floattoIEEMantissa(result));
    hexWriteIn(ui->resulHex, ConversorIEEE754::floattoIEESign(result), ConversorIEEE754::floattoIEEExp(result), ConversorIEEE754::floattoIEEMantissa(result));

}

float MainWindow::addOperation(float n1, float n2){
    //Pasos previos

    unsigned int expo1 = ConversorIEEE754::floattoIEESign(n1);
    unsigned int expo2 = ConversorIEEE754::floattoIEESign(n2);

    unsigned int signo1 = ConversorIEEE754::floattoIEEExp(n1);
    unsigned int signo2 = ConversorIEEE754::floattoIEEExp(n2);

    unsigned int mantissa1 = ConversorIEEE754::floattoIEEMantissa(n1) + bitPos.at(23);
    unsigned int mantissa2 = ConversorIEEE754::floattoIEEMantissa(n2) + bitPos.at(23);


//    const unsigned int excsBits = bitPos.at(31)+bitPos.at(30)+bitPos.at(29)+bitPos.at(28)+bitPos.at(27)+bitPos.at(26)+bitPos.at(25)+bitPos.at(24);

    //1. Inicializar guarda, sticky, round, operandos y complementado.
    unsigned int signoSuma;

    unsigned int g = 0; unsigned int r = 0; unsigned int st = 0;
    bool interOp = false;
    bool cP = false;

    //2. Comprobar exponentes.
    if(expo1 < expo2){
        expo1 = ConversorIEEE754::floattoIEEExp(n2);
        expo2 = ConversorIEEE754::floattoIEEExp(n1);
        mantissa1 = ConversorIEEE754::floattoIEEMantissa(n2) + bitPos.at(23);
        mantissa2 = ConversorIEEE754::floattoIEEMantissa(n1) + bitPos.at(23);
        signo1 = ConversorIEEE754::floattoIEESign(n2);
        signo2 = ConversorIEEE754::floattoIEESign(n1);
        interOp = true;
    }
    //3. Calculamos exponente de la suma y de "d".
    int addExpo = expo1;
    unsigned int d = expo1 - expo2;
    //4. Comprobamos si los signos son diferentes.

    if(signo1!=signo2) mantissa2 = (~mantissa2)%0b1000000000000000000000000+1;

    //5. Creamos P.
    unsigned int P = mantissa2;
    //6. Establecemos los bits de guarda, sticky y round.
    if(d>=3 && d < 25){
        g = (bitPos.at(d-1)&P)!=0;
        r = (bitPos.at(d-2)&P)!=0;
        st = (bitPos.at(d-3)&P)!=0;
    }
    for(int i = d-3; i > 0 && !st; i--) st = st|(bitPos.at(d-i)&P);
    //7. Comprobamos de nuevo los signos.
    if(signo1!=signo2){
        for(unsigned int i = 0; i < d; i++){
            P >>= 1;
            P += bitPos.at(23);
        }
    }
    else P = P>>d;
    //8. Sacamos el acarreo.
    unsigned int C = 0;
    C = acarreo(mantissa1, P, 0, 0);
    P = mantissa1 + P;

    //9. Comprobamos el signo, la P y el acarreo.
    if(signo1!=signo2 && (P & bitPos.at(23)) != 0 && C == 0){
        P = (~P)%0b1000000000000000000000000+1;
        cP = true;
    }

    //10. Asimismo, comprobamos si el signo es el mismo y el acarreo es 1.
    if(signo1 == signo2 && C==1){

        st = g | r | st;

        r = P%2;

        P = P>>1;
        P += bitPos.at(23);

        addExpo++;

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

        addExpo -= k;

        if(P == 0) addExpo = 0;

    }

    //11. Redondeamos P.
    if((r==1 && st == 1)||(r==1 && st == 0 && P%2 == 1)){
        unsigned int CC = calcularAcarreo(P, 0b100000000000000000000,0,0);
        P = P+1;

        if(CC) {
            P >>= 1;
            P += bitPos.at(23);
            addExpo++;
        }
    }

    //12. Calculamos el signo del resultado

    signoSuma = (!interOp && cP) ? signo2:signo1;

    //Denormales
    if(addExpo>0b11111111){
        return (signoSuma)? -Q_INFINITY:Q_INFINITY;
    }
    else if(addExpo<0){

        unsigned int t = 1 - addExpo;

        P >>=t;

        addExpo = 0;

    }

    return ConversorIEEE754::IEEtofloat(signoSuma, addExpo, P);

}

void MainWindow::on_multiplicacion_clicked()
{

    float n1 = ui->op1Real->text().toFloat();

    float n2 = ui->op1Real->text().toFloat();

    float result = multiplyOperation(n1, n2);

    binaryWriteIn( ui->op1IEEE, ConversorIEEE754::floattoIEESign(n1), ConversorIEEE754::floattoIEEExp(n1), ConversorIEEE754::floattoIEEMantissa(n1));
    hexWriteIn(ui->op1Hex,  ConversorIEEE754::floattoIEESign(n1), ConversorIEEE754::floattoIEEExp(n1), ConversorIEEE754::floattoIEEMantissa(n1));

    binaryWriteIn( ui->op2IEEE,  ConversorIEEE754::floattoIEESign(n2), ConversorIEEE754::floattoIEEExp(n2), ConversorIEEE754::floattoIEEMantissa(n2));
    hexWriteIn(ui->op2Hex, ConversorIEEE754::floattoIEESign(n2), ConversorIEEE754::floattoIEEExp(n2), ConversorIEEE754::floattoIEEMantissa(n2));

    ui->resulReal->setText(QString::fromStdString(std::to_string(result)));
    binaryWriteIn(ui -> resulIEEE, ConversorIEEE754::floattoIEESign(result), ConversorIEEE754::floattoIEEExp(result), ConversorIEEE754::floattoIEEMantissa(result));
    hexWriteIn(ui->resulHex, ConversorIEEE754::floattoIEESign(result), ConversorIEEE754::floattoIEEExp(result), ConversorIEEE754::floattoIEEMantissa(result));


}

float MainWindow::multiplyOperation(float n1, float n2){

    unsigned int signo1 = ConversorIEEE754::floattoIEESign(n1);
    unsigned int signo2 = ConversorIEEE754::floattoIEESign(n2);

    unsigned int expo1 = ConversorIEEE754::floattoIEEExp(n1);
    unsigned int expo2 = ConversorIEEE754::floattoIEEExp(n2);

    unsigned int mantissa1 = ConversorIEEE754::floattoIEEMantissa(n1) + bitPos.at(23);
    unsigned int mantissa2 = ConversorIEEE754::floattoIEEMantissa(n2) + bitPos.at(23);

    //Paso 1:
    unsigned int signoResul = signo1 ^ signo2;
    //Paso 2:
    int expResul = expo1 + expo2  - 0b1111111;

    //Paso 3:
    //Paso 3i:
    unsigned int c = 0;
    unsigned int P = 0;
    unsigned int A = mantissa1;

    for(int i = 0; i < 24; i++){
            P+=A%2*mantissa2;
            A = (A>>1) + (P%2)*bitPos.at(23);
            P = (P>>1) + c*bitPos.at(23);
            c>>=1;
    }

    //Paso 3ii:

    if((P & bitPos.at(23))==0){
        P <<= 1;
    }
    else{
        expResul++;
    }

    //Paso 3iii:
    unsigned int r = (A & bitPos.at(23))!= 0;

    //Paso 3iv:
    unsigned int st = 0;
    for(int i = 0; i < 23; i++) st |= (A & bitPos.at(i))!= 0;

    //Paso 3v:
    if((r&&st) ||(r&&!st&&P%2)) P+=1;

    //DESBORDAMIENTOS
    if(expResul>0b11111111){
        return (signoResul)? -Q_INFINITY:Q_INFINITY;
    }
    else if(expResul<0){

        unsigned int t = 1 - expResul;

        P >>=t;

        expResul = 0;

    }

    return ConversorIEEE754::IEEtofloat(signoResul, expResul, P);

}

void MainWindow::on_division_clicked()
{
    float n1 = ui->op1Real->text().toFloat();
    float n2 = ui->op2Real->text().toFloat();

    //Pasos previos

    unsigned int signo1 = ConversorIEEE754::floattoIEESign(n1);
    unsigned int signo2 = ConversorIEEE754::floattoIEESign(n2);

    unsigned int expo1 = ConversorIEEE754::floattoIEEExp(n1);
    unsigned int expo2 = ConversorIEEE754::floattoIEEExp(n2);

    unsigned int mantissa1 = ConversorIEEE754::floattoIEEMantissa(n1);// + bitPos.at(23);
    unsigned int mantissa2 = ConversorIEEE754::floattoIEEMantissa(n2);// + bitPos.at(23);






}

float MainWindow::denormalCalculator(unsigned int sign, unsigned int mantissa){
    float preMantissa =1-ConversorIEEE754::IEEtofloat(0,127,mantissa);
    if(sign){
        return mantissa * 1.1754944e-38;//2^-126 para la alu
    }else{
        return mantissa * -1.1754944e-38;//-2^-126 para la alu
    }
}

void MainWindow::bNumWrite(QLineEdit* objetive, unsigned int signo, unsigned int expo, unsigned int mantissa)
{
    QString bNum;

    for(int i =0;i< 23;i++){
        bNum.push_front(QString::fromStdString(std::to_string(mantissa%2)));
        mantissa/=2;
    }

    for(int i=0;i<8;i++){
        bNum.push_front(QString::fromStdString(std::to_string(expo%2)));
        expo /= 2;
    }

    bNum.push_front(QString::fromStdString(std::to_string(signo)));


    bNum.push_front("0b");

    objetive->setText(bNum);
}

unsigned int MainWindow::acarreo(unsigned int mantissa1, unsigned int mantissa2, unsigned int position, unsigned int acarreoAc)
{
    //Caso base.
    if(position == 24) return acarreoAc;
    //Comprobaciones.
    if((mantissa2 & bitPos.at(position)) != 0 && (mantissa1 & bitPos.at(position)) != 0) return calcularAcarreo(mantissa1, mantissa2, position+1, 1);
    else if(((mantissa2 & bitPos.at(position)) != 0 || (mantissa1 & bitPos.at(position)) != 0) && acarreoAc != 0) return calcularAcarreo(mantissa1, mantissa2, position+1, 1);
    else return calcularAcarreo(mantissa1, mantissa2, position+1, 0);

}

void MainWindow::on_pushButtom_clicked()
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

void MainWindow::hexNumWrite(QLineEdit* child, unsigned int sign, unsigned int exp, unsigned int mantissa){

    QString stringHex;

    unsigned int aux = (sign << 31) + (exp << 23) + (mantissa);

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

