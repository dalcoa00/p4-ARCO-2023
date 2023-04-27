#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

