//=================================================================================================
//
// Application starter (c) Pagony Multimedia Studio Bt - 2014
//
//=================================================================================================
//
// Filename    : mainwindow.cpp
// AppVersion  : 1.0
// FileVersion : 1.0
// Authors     : Bikfalvi Tamas
//
//=================================================================================================

#include <QApplication>
#include <QMessageBox>
#include <QTranslator>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QSettings>

//-------------------------------------------------------------------------------------------------

#include "mainwindow.h"

//-------------------------------------------------------------------------------------------------

QApplication    *apMainApp;
QTranslator     *poTransSetup;
QTranslator     *poTransQT;

//=================================================================================================
int main(int argc, char *argv[])
//-------------------------------------------------------------------------------------------------
{
    //---------------------------------------------------------------------------------------------
    // Main application instance
    apMainApp = new QApplication(argc, argv);

    QString m_qsPathAppHome = QDir::currentPath();

    // Remove trailing dir separator
    m_qsPathAppHome.replace( "\\", "/" );
    if( m_qsPathAppHome.right(1).compare("/") == 0 )
    {
        m_qsPathAppHome = m_qsPathAppHome.left( m_qsPathAppHome.length()-1 );
    }

    //---------------------------------------------------------------------------------------------
    // Reading preferences from settings.ini file
    QSettings   obPrefFile( "settings.ini", QSettings::IniFormat );
    QString     qsLangPath  = obPrefFile.value( QString::fromAscii( "Language/Path" ), "" ).toString();
    QString     qsLang      = obPrefFile.value( QString::fromAscii( "Language/Extension" ), "en" ).toString();
    int         inWidth     = obPrefFile.value( QString::fromAscii( "Settings/WindowWidth" ), 640 ).toInt();
    int         inHeight    = obPrefFile.value( QString::fromAscii( "Settings/WindowHeight" ), 400 ).toInt();
    QString     qsTextColor = obPrefFile.value( QString::fromAscii( "Settings/Textcolor" ), "000000" ).toString();
    int         nTimerMs    = obPrefFile.value( QString::fromAscii( "Settings/Timer" ), 500 ).toInt();

    //---------------------------------------------------------------------------------------------
    // Loading translations based on ini file language settings
    poTransSetup = new QTranslator();
    poTransQT = new QTranslator();

    if( qsLangPath.length() > 0 )   {   qsLangPath.append( "/" );  }

    QString qsPathLangApp   = QString("%1/%2starter_%3.qm").arg(m_qsPathAppHome).arg(qsLangPath).arg(qsLang).replace( "\\", "/" ).replace( "//", "/" );
    QString qsPathLangQt    = QString("%1/%2qt_%3.qm").arg(m_qsPathAppHome).arg(qsLangPath).arg(qsLang).replace( "\\", "/" ).replace( "//", "/" );

    poTransSetup->load( qsPathLangApp );
    poTransQT->load( qsPathLangQt );

    apMainApp->installTranslator( poTransSetup );
    apMainApp->installTranslator( poTransQT );

    //---------------------------------------------------------------------------------------------
    // Main window instance
    MainWindow wndMain;

    wndMain.resize( inWidth, inHeight );
    wndMain.setProgressTextColor( qsTextColor );
    wndMain.setTimerIntervall( nTimerMs );
    wndMain.setAppHomeDirectory( m_qsPathAppHome );
    wndMain.init();

    wndMain.show();

    //---------------------------------------------------------------------------------------------
    return apMainApp->exec();
    //---------------------------------------------------------------------------------------------
}

//=================================================================================================
