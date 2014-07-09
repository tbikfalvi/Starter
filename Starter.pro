#-------------------------------------------------
#
# Project created by QtCreator 2014-04-18T15:33:07
#
#-------------------------------------------------

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT          += core gui xml network
TARGET       = Starter
TEMPLATE     = app
RC_FILE      = starter.rc
RESOURCES   += starter.qrc

TRANSLATIONS = starter_hu.ts \
               qt_hu.ts

SOURCES     += main.cpp \
               mainwindow.cpp

HEADERS     += mainwindow.h

FORMS       += mainwindow.ui
