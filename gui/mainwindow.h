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
    void on_pushButton_close_clicked();
    void on_pushButton_open_clicked();
    void on_pushButton_reload_clicked();
    void on_pushButton_create_clicked();
    void on_pushButton_explore_clicked();
    void on_pushButton_zoomin_clicked();
    void on_pushButton_zoomout_clicked();
    void on_pushButton_setmax_clicked();
    void on_pushButton_creategraph_clicked();
    void on_pushButton_refresh_clicked();
    void on_pushButton_test_clicked();
};

#endif // MAINWINDOW_H
