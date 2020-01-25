/**
 * @file main.cpp
 * @author Patrick Willner (patrick.willner@tum.de), Andreas Koliopoulos (ga96leh@mytum.de), Alexander Schmaus (ga96fin@mytum.de)
 * @brief
 * @version 1.0
 * @date 2019-12-12
 *
 * @copyright Copyright (c) 2019
 *
 */
#include <QApplication>
#include "mainwindow.h"

/**
 * @brief main function, initializes the application window.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
