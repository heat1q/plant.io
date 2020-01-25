/**
 * @file mainwindow.h
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief
 * @version 1.0
 * @date 2019-12-12
 *
 * @copyright Copyright (c) 2019
 *
 */
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

/**
 * @brief The MainWindow class
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    /**
     * @brief Main Window setup function. Scans usb ports for motes on startup.
     * Paints textEdit background black and text color white for an authentic console experience.
     */
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    /**
     * @brief changeEvent
     * @param e
     */
    void changeEvent(QEvent *e);

    Ui::MainWindow *ui;
    QextSerialPort port;
    QMessageBox error;
    QGraphicsScene* mScene;

    /**
     * @brief Reset all arrays associated with the topology visualization graph.
     */
    void reset_graph();

    /**
     * @brief Function for creating visualization of the sensor network topology.
     * Every time a route is received this function is called to plot the route.
     * @param InputList The optimal route from the respective mote to the gui
     */
    void create_graph(QStringList list);

    /**
     * @brief Send serial commands to the connected mote.
     * @param input The command that should be sent over serial communication.
     */
    void send2port(QString msg);

    /**
     * @brief Helper function for outputting messages to the console.
     * @param msg Message to be printed.
     */
    void print(QString msg);

    /**
     * @brief Function for plotting received sensor data.
     * @param type Type of sensor data: 1 = Temperature, 2 = Humidity, 3 = Light
     * @param data Sensor data to be plotted
     * @param id Associated mote id
     */
    void plot(int type, QStringList data, QString id);

    /**
     * @brief Function for sending a certain serial request to all selected motes from the corresponding listWidget.
     * @param listWidget The listWidget where the id selection is extracted from
     * @param requestType The type of serial communication request that should be sent to all selected motes
     */
    void send2selection(QListWidget* listWidget, QString requestType);

protected slots:
    /**
     * @brief Function for handling the retransmissions of unacknowledged requests.
     * After three tries this function recognizes that one or more routes have become invalid and thus
     * a new network exploration is triggered.
     */
    void resend2selection();

    /**
     * @brief Function for handling serial messages from the mote.
     * LT/GT operators <> denote important data and are simultaneously used as acknowledgements.
     * Requests from gui that natively don't solicit reply messages return an explicit ACK packet with the <> format.
     * This gives us consistant and reliable acknowledgements.
     */
    void receive();

    /**
     * @brief Function for enabling the exploration pushButton after a timeout.
     * Gets called in on_pushButton_Explore_clicked().
     */
    void enableButton();

    /**
     * @brief Opening the selected USB port.
     */
    void on_pushButton_Open_clicked();

    /**
     * @brief Closing the currently open USB port.
     */
    void on_pushButton_Close_clicked();

    /**
     * @brief Function for reloading/refreshing the available ports.
     */
    void on_pushButton_Reload_clicked();

    /**
     * @brief Initialize the network exploration.
     */
    void on_pushButton_Explore_clicked();

    /**
     * @brief Zoom in the netowork graph.
     */
    void on_pushButton_ZoomIn_clicked();

    /**
     * @brief Zoom out the network graph.
     */
    void on_pushButton_ZoomOut_clicked();

    /**
     * @brief Set the maximum amount of motes in the network.
     * This helps scaling the ACK timeout and network visualization.
     */
    void on_pushButton_SetMax_clicked();

    /**
     * @brief Refreshes the listWidget items on tab 2.
     */
    void on_pushButton_Refresh_Tab2_clicked();

    /**
     * @brief Selects all listWidget items on tab 2.
     */
    void on_pushButton_SelectAll_Tab2_clicked();

    /**
     * @brief Unselects all listWidget items on tab 2.
     */
    void on_pushButton_UnselectAll_Tab2_clicked();

    /**
     * @brief Refreshes the listWidget items on tab 3.
     */
    void on_pushButton_Refresh_Tab3_clicked();

    /**
     * @brief Selects all listWidget items on tab 3.
     */
    void on_pushButton_SelectAll_Tab3_clicked();

    /**
     * @brief Unselects all listWidget items on tab 3.
     */
    void on_pushButton_UnselectAll_Tab3_clicked();

    /**
     * @brief Configure temperature thresholds.
     */
    void on_pushButton_SendTemp_clicked();

    /**
     * @brief Configure humidity thresholds.
     */
    void on_pushButton_SendHum_clicked();

    /**
     * @brief Configure light thresholds.
     */
    void on_pushButton_SendLight_clicked();

    /**
     * @brief Configure all thresholds.
     */
    void on_pushButton_SendAll_clicked();

    /**
     * @brief Fetch all thresholds.
     */
    void on_pushButton_GetAll_clicked();

    /**
     * @brief Center and rescale the network topology graph with a small border for improved aesthetics.
     */
    void on_pushButton_Center_clicked();

    /**
     * @brief Clear console.
     */
    void on_pushButton_Clear_clicked();

    /**
     * @brief Function for initializing the sensor plots and requesting the sensor data from the selection.
     */
    void on_pushButton_GetSensorData_clicked();

    /**
     * @brief Get routing tables.
     */
    void on_pushButton_GetRoutingTable_clicked();

    /**
     * @brief Set LED colors.
     */
    void on_pushButton_LED_clicked();
};

#endif // MAINWINDOW_H
