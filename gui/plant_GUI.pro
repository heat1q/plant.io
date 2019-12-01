QT       += core gui
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = T_pH_Measuring
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        targetsettingswindow.cpp

HEADERS  += mainwindow.h \
    targetsettingswindow.h

FORMS    += mainwindow.ui \
    targetsettingswindow.ui

# include QextSerialPort
include(qextserialport/src/qextserialport.pri)
