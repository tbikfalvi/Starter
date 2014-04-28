#-------------------------------------------------
#
# Project created by QtCreator 2014-04-18T15:33:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Starter
TEMPLATE = app

TARGET       = Starter
RC_FILE      = starter.rc

RESOURCES   += starter.qrc

TRANSLATIONS = starter_en.ts \
               starter_hu.ts \
               qt_hu.ts

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
