#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include "ieee754converter.h"

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

    void binaryWriteIn(QLineEdit* child,  unsigned int sign, unsigned int exp, unsigned int mantisa);

    unsigned int calcularAcarreo(unsigned int manA, unsigned int manB, unsigned int pos, unsigned int acarreoActual);

    void hexWriteIn(QLineEdit* child,  unsigned int sign, unsigned int exp, unsigned int mantisa);

    QString toMantisa(unsigned int mantisa);

    Ui::MainWindow *ui;

    std::vector<unsigned int> bitPos;
};
#endif // MAINWINDOW_H
