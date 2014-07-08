#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QTranslator>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QSettings>

QApplication    *apMainApp;
QTranslator     *poTransSetup;
QTranslator     *poTransQT;
QString          g_qsCurrentPath;

int main(int argc, char *argv[])
{
    apMainApp = new QApplication(argc, argv);

    QSettings   obPrefFile( "settings.ini", QSettings::IniFormat );
    QString     qsLangPath  = obPrefFile.value( QString::fromAscii( "Language/Path" ), "" ).toString();
    QString     qsLang      = obPrefFile.value( QString::fromAscii( "Language/Extension" ), "hu" ).toString();
    int         inWidth     = obPrefFile.value( QString::fromAscii( "Settings/WindowWidth" ), 640 ).toInt();
    int         inHeight    = obPrefFile.value( QString::fromAscii( "Settings/WindowHeight" ), 400 ).toInt();

    poTransSetup = new QTranslator();
    poTransQT = new QTranslator();

    if( qsLangPath.length() > 0 )
    {
        qsLangPath.append( "\\" );
    }

    poTransSetup->load( QString("%1\\%2starter_%3.qm").arg(QDir::currentPath()).arg(qsLangPath).arg(qsLang) );
    poTransQT->load( QString("%1\\%2qt_%3.qm").arg(QDir::currentPath()).arg(qsLangPath).arg(qsLang) );

    apMainApp->installTranslator( poTransSetup );
    apMainApp->installTranslator( poTransQT );

    MainWindow wndMain;

    wndMain.resize( inWidth, inHeight );

    wndMain.show();

    return apMainApp->exec();
}
