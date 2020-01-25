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
