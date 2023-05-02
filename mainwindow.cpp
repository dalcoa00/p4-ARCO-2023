#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    bitPos.push_back(1);
    for(int i = 1; i < 32; i++){
        bitPos.push_back(2 * bitPos.at(i - 1));
    }

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
    float n1 = ui -> op1Real -> text().toFloat();
    float n2 = ui -> op2Real -> text().toFloat();

    float result = addOperation(n1, n2);

    bNumWrite(ui -> op1IEEE, IEEE754Converter::floatToIEESign(n1), IEEE754Converter::floatToIEEExp(n1), IEEE754Converter::floatToIEEMantissa(n1));
    hexNumWrite(ui -> op1Hex, IEEE754Converter::floatToIEESign(n1), IEEE754Converter::floatToIEEExp(n1), IEEE754Converter::floatToIEEMantissa(n1));

    bNumWrite(ui -> op2IEEE, IEEE754Converter::floatToIEESign(n2), IEEE754Converter::floatToIEEExp(n2), IEEE754Converter::floatToIEEMantissa(n2));
    hexNumWrite(ui -> op2Hex, IEEE754Converter::floatToIEESign(n2), IEEE754Converter::floatToIEEExp(n2), IEEE754Converter::floatToIEEMantissa(n2));

    ui->resulReal->setText(QString::fromStdString(std::to_string(result)));
    bNumWrite(ui -> resulIEEE, IEEE754Converter::floatToIEESign(result), IEEE754Converter::floatToIEEExp(result), IEEE754Converter::floatToIEEMantissa(result));
    hexNumWrite(ui -> resulHex, IEEE754Converter::floatToIEESign(result), IEEE754Converter::floatToIEEExp(result), IEEE754Converter::floatToIEEMantissa(result));
}

float MainWindow::addOperation(float n1, float n2)
{
    unsigned int expo1 = IEEE754Converter::floatToIEEExp(n1);
    unsigned int expo2 = IEEE754Converter::floatToIEEExp(n2);

    unsigned int signo1 = IEEE754Converter::floatToIEESign(n1);
    unsigned int signo2 = IEEE754Converter::floatToIEESign(n2);

    unsigned int mantissa1 = IEEE754Converter::floatToIEEMantissa(n1) + bitPos.at(23);
    unsigned int mantissa2 = IEEE754Converter::floatToIEESign(n2) + bitPos.at(23);

    const unsigned int excesoBits = bitPos.at(31) + bitPos.at(30) + bitPos.at(29) + bitPos.at(28) + bitPos.at(27) + bitPos.at(26) + bitPos.at(25) + bitPos.at(24);

    //1. Inicializar guarda, sticky, round, operandos y complementado.

    unsigned int signoSuma, g = 0, r = 0, st = 0;
    bool interOp = false, cP = false;

    //2. Comprobar exponentes.

    if(expo1 < expo2)
    {
        expo1 = IEEE754Converter::floatToIEEExp(n2);
        expo2 = IEEE754Converter::floatToIEEExp(n1);
        mantissa1 = IEEE754Converter::floatToIEEMantissa(n2) + bitPos.at(23);
        mantissa2 = IEEE754Converter::floatToIEEMantissa(n1) + bitPos.at(23);
        signo1 = IEEE754Converter::floatToIEESign(n2);
        signo2 = IEEE754Converter::floatToIEESign(n1);

//        unsigned int auxExpo = expo1;
//        unsigned int auxSigno = signo1;
//        unsigned int auxMant = mantissa1;
//        expo1 = expo2; signo1 = signo2; mantissa1 = mantissa2;
//        expo2 = auxExpo; signo2 = auxSigno; mantissa2 = auxMant;
        interOp = true;
    }

    //3. Calculamos exponente de la suma y de "d".

    int addExpo = expo1;
    unsigned int d = expo1 - expo2;

    //4. Comprobamos si los signos son diferentes.

    if(signo1 != signo2)
    {
        mantissa2 = (~mantissa2)%0b1000000000000000000000000+1;
    }

    //5. Creamos P.

    unsigned int P = mantissa2;

    //6. Establecemos los bits de guarda, sticky y round.

    if(d >= 3 && d < 25)
    {
        g = (bitPos.at(d - 1) &P) != 0;
        r = (bitPos.at(d - 2) &P) != 0;
        st = (bitPos.at(d - 3) &P) != 0;
    }

    for(int i = d - 3; !st && i > 0; i--)
    {
        st = st|(bitPos.at(d - i) &P);
    }

    //7. Comprobamos de nuevo los signos.

    if(signo1 != signo2)
    {
        for(unsigned int i = 0; i < d; i++)
        {
            P >>= 1;
            P += bitPos.at(23);
        }

    }else{
        P = P >> d;
    }

    //8. Sacamos el acarreo.
    unsigned int C = 0;

    C = acarreo(mantissa1, P, 0, 0);
    P += mantissa1;

    //9. Comprobamos el signo, la P y el acarreo.

    if(signo1 != signo2 && (P & bitPos.at(23)) != 0 && C == 0)
    {
        P = (~P)%0b1000000000000000000000000+1;
        cP = true;
    }

    //10. Asimismo, comprobamos si el signo es el mismo y el acarreo es 1.

    if(signo1 == signo2 && C == 1)
    {
        st = g|r|st;
        r = P % 2;

        P = P >> 1;
        P += bitPos.at(23);

        addExpo++;
    }else{
        int aux = 0;

        for(unsigned int i = P; i != 0 && (i & bitPos.at(23)) == 0; i <<= 1){
            aux++;
        }

        if(aux == 0)
        {
            st = r|st;
            r = g;
        }else{
            r = 0;
            st = 0;
        }

        for(int i = 0; i < aux; i++)
        {
            P <<= 1;
            P += g;
        }

        addExpo -= aux;

        if(P == 0)
        {
            addExpo = 0;
        }
    }

    //11. Redondeamos P.

    if((st == 1 && r == 1) || (r == 1 && st == 0 && P % 2 == 1))
    {
        unsigned int CC = acarreo(P, 0b100000000000000000000, 0, 0);
        P += 1;

        if(CC)
        {
            P >>= 1;
            P += bitPos.at(23);
            addExpo++;
        }
    }

    //12. Calculamos el signo del resultado

    signoSuma = (!interOp && cP) ? signo2 : signo1;

    //Denormales

    if(addExpo > 0b11111111)
    {
        return (signoSuma) ? -Q_INFINITY:Q_INFINITY;

    }else if(addExpo < 0){
        unsigned int T = 1 - addExpo;

        P >>= T;

        addExpo = 0;
    }

    return IEEE754Converter::IEEtoFloat(signoSuma, addExpo, P);

}


/************************************ MULTIPLICACIÓN ************************************/



/************************************ DIVISIÓN ************************************/



/************************************ MÉTODOS AUXILIARES ************************************/
unsigned int MainWindow::acarreo(unsigned int mantissa1, unsigned int mantissa2, unsigned int position, unsigned int acarreoAc)
{
    //Caso base.
    if (position == 24)
    {
        return acarreoAc;
    }

    //Comprobaciones.
    if ((mantissa2 & bitPos.at(position)) != 0 && (mantissa1 & bitPos.at(position)) != 0) {

        return acarreo(mantissa1, mantissa2, position + 1, 1);
    }
    else if ((acarreoAc != 0 && (mantissa1 & bitPos.at(position)) != 0) || (mantissa2 & bitPos.at(position)) != 0){

        return acarreo(mantissa1, mantissa2, position + 1, 1);
    }
    else {

        return acarreo(mantissa1, mantissa2, position + 1, 0);
    }
}

void MainWindow::bNumWrite(QLineEdit* objective, unsigned int signo, unsigned int expo, unsigned int mantissa)
{
    QString bNum;

    for(int i = 0; i < 23; i++)
    {
        bNum.push_front(QString::fromStdString(std::to_string(mantissa % 2)));
        mantissa /= 2;
    }

    for(int i = 0; i < 8; i++)
    {
        bNum.push_front(QString::fromStdString(std::to_string(expo % 2)));
        expo /= 2;
    }

    bNum.push_front(QString::fromStdString(std::to_string(signo)));

    bNum.push_front("0b");
    objective -> setText(bNum);
}

void MainWindow::hexNumWrite(QLineEdit* objective, unsigned int signo, unsigned int expo, unsigned int mantissa)
{
    QString hexNum;

    unsigned int chain = (signo << 31) + (expo << 23) + (mantissa);

    for(int i = 0; i < 8; i++)
    {
        unsigned int mod = chain % 16;

        switch (mod) {

        case 10:
            hexNum.push_front('A');
            break;
        case 11:
            hexNum.push_front('B');
            break;
        case 12:
            hexNum.push_front('C');
            break;
        case 13:
            hexNum.push_front('D');
            break;
        case 14:
            hexNum.push_front('E');
            break;
        case 15:
            hexNum.push_front('F');
            break;

        default:
            hexNum.push_front(QString::fromStdString(std::to_string(mod)));
        }


        chain >>= 4;
    }

    objective -> setText("0x" + hexNum);
}

//char MainWindow::hexCheck(int check)
//{
//    switch(check)
//    {
//    case 10:
//        return 'A';
//    case 11:
//        return 'B';
//    case 12:
//        return 'C';
//    case 13:
//        return 'D';
//    case 14:
//        return 'E';
//    case 15:
//        return 'F';
//    default:

//    }
//}

//bool MainWindow::hexBoolCheck(int check)
//{
//    return check == 10 || check == 11 || check == 12 || check == 13 || check == 14 || check == 15;
//}

void MainWindow::on_pushButton_clicked()
{
    ui->op1Real->setText("");
    ui->op2Real->setText("");
    ui->op1IEEE->setText("");
    ui->op1Hex->setText("");
    ui->op2IEEE->setText("");
    ui->op2Hex->setText("");
    ui->resulReal->setText("");
    ui->resulIEEE->setText("");
    ui->resulHex->setText("");
}

