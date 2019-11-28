QT       += core gui
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = AmbientTemperature
TEMPLATE = app


SOURCES += main.cpp\
        edge.cpp \
        mainwindow.cpp \
        networkgraph.cpp \
        node.cpp \
        targetsettingswindow.cpp

HEADERS  += mainwindow.h \
    edge.h \
    networkgraph.h \
    node.h \
    targetsettingswindow.h

FORMS    += mainwindow.ui \
    targetsettingswindow.ui

# include QextSerialPort
include(qextserialport/src/qextserialport.pri)
