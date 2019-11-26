#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include "qextserialport.h"
#include "qextserialenumerator.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    QextSerialPort port;
    QMessageBox error;

private slots:


private slots:
    void on_pushButton_close_clicked();
    void on_pushButton_open_clicked();
    void receive();
    void on_pushButton_clicked();
    void on_pushButton_send_clicked();
    void on_verticalSlider_valueChanged(int value);
    void on_pushButton_send_threshold_t_clicked();
    void on_pushButton_send_threshold_ph_clicked();
};

#endif // MAINWINDOW_H
