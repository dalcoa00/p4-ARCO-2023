#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include "conversorieee754.h"

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

    float denormalCalculator(unsigned int signo, unsigned int mantissa);
    
    float addOperation(float n1,float n2);

    float multiplyOperation(float n1, float n2);

    void bNumWrite(QLineEdit* objetive,  unsigned int signo, unsigned int expo, unsigned int mantissa);

    unsigned int acarreo(unsigned int mantissa1, unsigned int mantissa2, unsigned int posirion, unsigned int acarreoAc);

    void hexNumWrite(QLineEdit* objetive,  unsigned int signp, unsigned int expo, unsigned int mantissa);

    Ui::MainWindow *ui;

    std::vector<unsigned int> bitPos;
};
#endif // MAINWINDOW_H
