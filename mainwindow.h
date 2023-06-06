#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include "ieee754converter.h"
#include <math.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_suma_clicked();

    void on_multiplicacion_clicked();

    void on_division_clicked();

    void on_Reset_clicked();

private:
    QString resultado;

    float denormalCalculator(unsigned int sign, unsigned int mantissa);
    
    float addOperation(float op1,float op2);

    float multiplyOperation(float op1, float op2);

    float divisionOperation(float op1, float op2);

    QString toIEEEString(unsigned int signo, unsigned int exponente, unsigned int mantisa);

    unsigned int carry(unsigned int manA, unsigned int manB, unsigned int pos, unsigned int acarreoActual);

    QString toHexadecimalString(unsigned int signo, unsigned int exponente, unsigned int mantisa);

    int calculateOverflow(int expResul);

    unsigned int getC2(unsigned int number);

    QString toMantisa(unsigned int mantisa);

    Ui::MainWindow *ui;

    std::vector<unsigned int> bits;
};
#endif // MAINWINDOW_H
