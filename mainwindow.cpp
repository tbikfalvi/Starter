//=================================================================================================
//
// Application starter (c) Pagony Multimedia Studio Bt - 2014
//
//=================================================================================================
//
// Filename    : mainwindow.cpp
// AppVersion  : 1.0
// FileVersion : 1.0
// Author      : Bikfalvi Tamas
//
//=================================================================================================

#include <QMessageBox>
#include <QCloseEvent>
#include <QFile>
#include <QSettings>

//=================================================================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"

//=================================================================================================
// MainWindow::MainWindow
//-------------------------------------------------------------------------------------------------
// Constructor of main window
// Initialization of variables and the application
//-------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
//-------------------------------------------------------------------------------------------------
{
    //---------------------------------------------------------------
    // Initialize the GUI
    ui->setupUi(this);

    ui->lblProgressText->setText( "" );

    ui->progressBar->setVisible( false );
    ui->progressBar->setMinimum( 0 );
    ui->progressBar->setMaximum( 100 );
    ui->progressBar->setValue( 0 );

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    QFile   file( "splash.png" );

    if( file.exists() )
    {
        ui->frmMain->setStyleSheet( "background: url(splash.png);" );
    }

    //---------------------------------------------------------------
    // Initialize the variables
    m_nTimer            = 0;
    m_bProcessFinished  = false;

    m_qsLang            = "";

    //---------------------------------------------------------------
    // Start the application process with the timer
    m_nTimer = startTimer( 1000 );
}
//=================================================================================================
// MainWindow::~MainWindow
//-------------------------------------------------------------------------------------------------
// Destructor of main window
// Delete variables
//-------------------------------------------------------------------------------------------------
MainWindow::~MainWindow()
//-------------------------------------------------------------------------------------------------
{
    delete ui;
}
//=================================================================================================
// on_pbExit_clicked
//-------------------------------------------------------------------------------------------------
// Executed when the Exit button (top right on main window) has been clicked
// Closes the application
//-------------------------------------------------------------------------------------------------
void MainWindow::on_pbExit_clicked()
//-------------------------------------------------------------------------------------------------
{
    close();
}
//=================================================================================================
// closeEvent
//-------------------------------------------------------------------------------------------------
// Executed when the application closes
// If the user clicked the exit button or the application started to closed before the
// main process finished a message box appeares to abort the close process
//-------------------------------------------------------------------------------------------------
void MainWindow::closeEvent( QCloseEvent *p_poEvent )
//-------------------------------------------------------------------------------------------------
{
    if( !m_bProcessFinished )
    {
        if( QMessageBox::question( this, tr("Question"),
                                   tr("Are you sure you want to abort the process?"),
                                   QMessageBox::Yes, QMessageBox::No ) == QMessageBox::No )
        {
            p_poEvent->ignore();
        }
    }
}
//=================================================================================================
// timerEvent
//-------------------------------------------------------------------------------------------------
// Executed when a timer started
//-------------------------------------------------------------------------------------------------
void MainWindow::timerEvent(QTimerEvent *)
//-------------------------------------------------------------------------------------------------
{
    killTimer( m_nTimer );

    if( m_bProcessFinished )
    {
        close();
    }
    else
    {
        processMain();
    }
}
//=================================================================================================
// processMain
//-------------------------------------------------------------------------------------------------
// Main process of the application
// If any error occures the process aborts
//-------------------------------------------------------------------------------------------------
void MainWindow::processMain()
//-------------------------------------------------------------------------------------------------
{
    if( _checkEnvironment() )
    {
    }

    m_bProcessFinished = true;
    m_nTimer = startTimer( 2000 );
}
//=================================================================================================
// _checkEnvironment
//-------------------------------------------------------------------------------------------------
// Checking the application environment such needed settings files, directory structure etc.
//-------------------------------------------------------------------------------------------------
bool MainWindow::_checkEnvironment()
//-------------------------------------------------------------------------------------------------
{
    _progressInit( 3, tr("Checking environment ...") );

    _progressStep();

    QFile   file( "settings.ini" );

    if( !file.exists() )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("The 'settings.ini' file is missing.\n\n"
                                 "Please create the file and fulfill it with proper data.\n"
                                 "For more information about settings file, check the manual\n"
                                 "or contact the application provider.") );
        return false;
    }
    if( !_readSettings() )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("The 'settings.ini' file content is corrupt.\n\n"
                                 "Please check the file and fulfill it with proper data.\n"
                                 "For more information about settings file, check the manual\n"
                                 "or contact the application provider.") );
        return false;
    }

    _progressStep();
    file.setFileName( "process.xml" );
    if( !file.exists() )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("The 'process.xml' file is missing.\n\n"
                                 "Please create the file and fulfill it with proper data.\n"
                                 "For more information about process file, check the manual\n"
                                 "or contact the application provider.") );
        return false;
    }

    _progressStep();

    return true;
}
//=================================================================================================
// _readSettings
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
bool MainWindow::_readSettings()
//-------------------------------------------------------------------------------------------------
{
    QSettings  obPrefFile( "settings.ini", QSettings::IniFormat );

    m_qsLang = obPrefFile.value( QString::fromAscii( "Lang" ), "hu" ).toString();

    return true;
}
//=================================================================================================
// _progressInit
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_progressInit(int p_nMax, QString p_qsText)
//-------------------------------------------------------------------------------------------------
{
    ui->progressBar->setVisible( true );
    ui->progressBar->setValue( 0 );
    ui->progressBar->setMaximum( p_nMax );
    _progressText( p_qsText );
}
//=================================================================================================
// _progressStep
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_progressStep()
//-------------------------------------------------------------------------------------------------
{
    if( ui->progressBar->value() == ui->progressBar->maximum() )
        return;

    ui->progressBar->setValue( ui->progressBar->value()+1 );
}
//=================================================================================================
// _progressText
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_progressText(QString p_qsText)
//-------------------------------------------------------------------------------------------------
{
    ui->lblProgressText->setText( p_qsText );
}
//-------------------------------------------------------------------------------------------------
