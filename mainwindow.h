#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
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
    void on_add_clicked();

    void on_mult_clicked();

    void on_div_clicked();

    void on_Reset_clicked();

private:
    QString resultado;

    float denormalCalculator(unsigned long sign, unsigned long mantissa);
    
    float addOperation(float num1,float num2);

    float multOperation(float num1, float num2);

    float divOperation(float num1, float num2);

    QString toIEEEString(unsigned long signo, unsigned long exponente, unsigned long mantisa);

    unsigned long carry(unsigned long aMant, unsigned long bMant, unsigned long pos, unsigned long actualCarry);

    void getMethod(int clicked, float num1, float num2);

    void print(float total, float num1, float num2, QString result2);

    QString toHexadecimalString(unsigned long signo, unsigned long exponente, unsigned long mantisa);

    int calculateOverflow(int expResul);

    unsigned long getC2(unsigned long number);

    QString toMantisa(unsigned long mantisa);

    Ui::MainWindow *ui;

    std::vector<unsigned long> bits;
};
#endif // MAINWINDOW_H
