#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QVarLengthArray>
#include <qdebug.h>
#include <math.h>
#include "qextserialport.h"
#include "qextserialenumerator.h"
#include "ui_mainwindow.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    QextSerialPort port;
    QMessageBox error;
    QGraphicsScene* mScene;
    void reset_graph();
    void create_graph(QStringList List);
    void make_plot();
    void send2port(QString Input);

private slots:
    void receive();
    void on_pushButton_Close_clicked();
    void on_pushButton_Open_clicked();
    void on_pushButton_Reload_clicked();
    void on_pushButton_CreateRoute_clicked();
    void on_pushButton_Explore_clicked();
    void on_pushButton_ZoomIn_clicked();
    void on_pushButton_ZoomOut_clicked();
    void on_pushButton_SetMax_clicked();
    void on_pushButton_creategraph_clicked();
    void on_pushButton_Refresh_clicked();
    void on_pushButton_SetTemp_clicked();
    void on_pushButton_SetHum_clicked();
    void on_pushButton_SetLight_clicked();
    void on_pushButton_SetAll_clicked();
    void on_pushButton_Debug_clicked();
    void on_pushButton_SendTemp_clicked();
    void on_pushButton_SendHum_clicked();
    void on_pushButton_SendLight_clicked();
    void on_pushButton_SendAll_clicked();
};

#endif // MAINWINDOW_H
