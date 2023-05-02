#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qlineedit.h"
#include <QMainWindow>
#include <ieee754converter.h>

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

    void on_pushButton_clicked();

    void on_multiplicacion_clicked();

private:
    void bNumWrite(QLineEdit* objective, unsigned int signo, unsigned int expo, unsigned int mantissa);
    void hexNumWrite(QLineEdit* objective, unsigned int signo, unsigned int expo, unsigned int mantissa);
    float addOperation(float n1, float n2);
    float multiplyOperation(float n1, float n2);
//    char hexCheck(int check);
//    bool hexBoolCheck(int check);
    unsigned int acarreo(unsigned int mantissa1, unsigned int mantissa2, unsigned int position, unsigned int acarreoAc);
    Ui::MainWindow *ui;
    std::vector<unsigned int> bitPos;
};
#endif // MAINWINDOW_H
