#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QDir>

QApplication    *apMainApp;
QTranslator     *poTransSetup;
QTranslator     *poTransQT;
QString          g_qsCurrentPath;

int main(int argc, char *argv[])
{
    apMainApp = new QApplication(argc, argv);

    poTransSetup = new QTranslator();
    poTransQT = new QTranslator();

    poTransSetup->load( QString("%1\\lang\\starter_hu.qm").arg(QDir::currentPath()) );
    poTransQT->load( QString("%1\\lang\\qt_hu.qm").arg(QDir::currentPath()) );

    apMainApp->installTranslator( poTransSetup );
    apMainApp->installTranslator( poTransQT );

    MainWindow wndMain;

    wndMain.show();

    return apMainApp->exec();
}
