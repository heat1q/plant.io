#-------------------------------------------------
#
# Project created by QtCreator 2014-09-18T17:36:31
#
#-------------------------------------------------

QT       += core gui
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AmbientTemperature
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        targetsettingswindow.cpp

HEADERS  += mainwindow.h \
    targetsettingswindow.h

FORMS    += mainwindow.ui \
    targetsettingswindow.ui

#-------------------------------------------------
# This section will include QextSerialPort in
# your project:

include(qextserialport/src/qextserialport.pri)

# Before running the project, run qmake first:
# In Qt Creator, right-click on the project
# and choose "run qmake".
# The qextserialport folder should appear in the
# directory tree.
# Now your project is ready to run.
#-------------------------------------------------
