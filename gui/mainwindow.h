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
#include "targetsettingswindow.h"
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

private slots:
    void on_pushButton_close_clicked();
    void on_pushButton_open_clicked();
    void receive();
    void on_pushButton_clicked();
    void on_pushButton_send_clicked();
    void on_verticalSlider_valueChanged(int value);
    void on_pushButton_send_threshold_t_clicked();
    void on_pushButton_send_threshold_ph_clicked();
    void on_pushButton_reload_clicked();
    void on_pushButton_create_clicked();
    void on_pushButton_explore_clicked();
    void on_pushButton_zoomin_clicked();
    void on_pushButton_zoomout_clicked();
    void on_pushButton_setmax_clicked();
};

#endif // MAINWINDOW_H
