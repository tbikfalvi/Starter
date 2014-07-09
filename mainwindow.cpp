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
#include <QDir>
#include <QUrl>
#include <QDate>

//=================================================================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"

// !!!TODO!!! <- megoldando feladat

//=================================================================================================
// MainWindow::MainWindow
//-------------------------------------------------------------------------------------------------
// Constructor of main window
// Initialization of variables and the application
//-------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
//-------------------------------------------------------------------------------------------------
{
    _log( tr("Start application\nInitialize variables\n") );

    //---------------------------------------------------------------
    // Initialize the variables
    m_nTimer            = 0;
    m_bProcessFinished  = false;
    m_qsLang            = "";
    m_qsCurrentDir      = QDir::currentPath();
    m_qsProcessFile     = "";
    m_nDownload         = 0;

    m_qsCurrentDir.replace( '/', '\\' );
    if( m_qsCurrentDir.right(1).compare("\\") == 0 )
    {
        m_qsCurrentDir = m_qsCurrentDir.left(m_qsCurrentDir.length()-1);
    }

    _log( tr("Create XML and Http objects\n") );

    obProcessDoc = new QDomDocument( "StarterProcess" );
    obHttp = new QHttp( this );

    connect(obHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(_slotHttpRequestFinished(int,bool)));
    connect(obHttp, SIGNAL(dataReadProgress(int, int)), this, SLOT(_slotUpdateDataReadProgress(int, int)));
    connect(obHttp, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(_slotReadResponseHeader(const QHttpResponseHeader &)));
    connect(obHttp, SIGNAL(authenticationRequired(const QString &, quint16, QAuthenticator *)), this, SLOT(_slotAuthenticationRequired(const QString &, quint16, QAuthenticator *)));
#ifndef QT_NO_OPENSSL
    connect(obHttp, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(_slotSslErrors(const QList<QSslError> &)));
#endif

    _log( tr("Initialize GUI elements\n") );

    //---------------------------------------------------------------
    // Initialize the GUI
    ui->setupUi(this);

    ui->lblProgressText->setText( "" );

    ui->progressBar->setVisible( false );
    ui->progressBar->setMinimum( 0 );
    ui->progressBar->setMaximum( 100 );
    ui->progressBar->setValue( 0 );

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    QSettings   obPrefFile( "settings.ini", QSettings::IniFormat );
    QString     qsFile = obPrefFile.value( QString::fromAscii( "Settings/Background" ), "" ).toString();
    QFile       qfFile( QString("%1\\%2").arg(m_qsCurrentDir).arg(qsFile) );

    if( qfFile.exists() )
    {
        ui->frmMain->setStyleSheet( QString( "QFrame { border-image: url(%1); }" ).arg(qsFile) );
    }

    ui->lblProgressText->setStyleSheet( "QLabel { border-image: url(none); }" );

    //---------------------------------------------------------------
    // Start the application process with the timer
    m_nTimer = startTimer( 1000 );

    _log( tr("End of initialization\n\n") );
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
    _log( tr("Exiting application ...\n\n") );
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
        _log( tr("Process finished, closing application ...\n") );
        close();
    }
    else
    {
        _log( tr("Start main process ...\n") );
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
    bool    bContinue = false;

    _log( tr("Checking environment ...\n") );
    bContinue = _checkEnvironment();

    _progressStep();

    if( bContinue )
    {
        _log( tr("Environment check SUCCEEDED\n") );

        if( m_qsDownloadAddress.length() > 0 && m_qsProcessFile.length() > 0 )
        {
            _log( tr("Download process XML file\n") );
            bContinue = _downloadProcessXML();

            _progressStep();

            if( bContinue ) bContinue = _readProcessXML();

            _progressStep();
        }
    }

    _log( tr("Main process finished\n") );

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

    QFile   qfFile( "settings.ini" );

    //-----------------------------------------------------------------------------------
    // STEP 1 - checking if ini file exists
    //-----------------------------------------------------------------------------------
    if( !qfFile.exists() )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("The 'settings.ini' file is missing.\n\n"
                                 "Please create the file and fulfill it with proper data.\n"
                                 "For more information about settings file, check the manual\n"
                                 "or contact the application provider.") );
        return false;
    }
    if( !readSettings() )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("The 'settings.ini' file content is corrupt.\n\n"
                                 "Please check the file and fulfill it with proper data.\n"
                                 "For more information about settings file, check the manual\n"
                                 "or contact the application provider.") );
        return false;
    }

    _progressStep();

    //-----------------------------------------------------------------------------------
    // STEP 2 - checking if backup directory exists
    //-----------------------------------------------------------------------------------
    QDir    qdBackup( QString("%1\\backup").arg(m_qsCurrentDir) );

    if( !qdBackup.exists() )
    {
        if( !qdBackup.mkpath( QString("%1\\backup").arg(m_qsCurrentDir) ) )
        {
            QMessageBox::warning( this, tr("Warning"),
                                  tr("The 'backup' directory not exists and can not be created.\n\n"
                                     "Please create the backup directory in the home directory"
                                     "of this application and restart it again.") );
            return false;
        }
    }

    _progressStep();

    //-----------------------------------------------------------------------------------
    // STEP 3 - checking if download directory exists
    //-----------------------------------------------------------------------------------
    QDir    qdDownload( QString("%1\\download").arg(m_qsCurrentDir) );

    if( !qdDownload.exists() )
    {
        if( !qdDownload.mkpath( QString("%1\\download").arg(m_qsCurrentDir) ) )
        {
            QMessageBox::warning( this, tr("Warning"),
                                  tr("The 'download' directory not exists and can not be created.\n\n"
                                     "Please create the backup directory in the home directory"
                                     "of this application and restart it again.") );
            return false;
        }
    }

    _progressStep();

    return true;
}
//=================================================================================================
// _readProcessXML
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
bool MainWindow::_downloadProcessXML()
//-------------------------------------------------------------------------------------------------
{

    return true;
}
//=================================================================================================
// _readProcessXML
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
bool MainWindow::_readProcessXML()
//-------------------------------------------------------------------------------------------------
{
    QFile qfFile( m_qsProcessFile );

    if( !qfFile.exists() )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("The '%1' file is missing.\n\n"
                                 "Please create the file and fulfill it with proper data.\n"
                                 "For more information about process file, check the manual\n"
                                 "or contact the application provider.").arg(m_qsProcessFile) );
        return false;
    }

    _progressStep();

    if( !qfFile.open(QIODevice::ReadOnly) )
    {
        QMessageBox::warning( this, tr("Warning"), tr("Unable to read 'process.xml' file.") );
        return false;
    }

    QString      qsErrorMsg  = "";
    int          inErrorLine = 0;

    qfFile.seek( 0 );
    if( !obProcessDoc->setContent( &qfFile, &qsErrorMsg, &inErrorLine ) )
    {
        QMessageBox::warning( this, tr("Warning"), tr( "Error occured during parsing file: process.xml\n\nError in line %2: %3" ).arg( inErrorLine ).arg( qsErrorMsg ) );
        qfFile.close();
        return false;
    }
    qfFile.close();

    _progressStep();

    return true;
}
//=================================================================================================
// readSettings
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
bool MainWindow::readSettings()
//-------------------------------------------------------------------------------------------------
{
    QSettings  obPrefFile( "settings.ini", QSettings::IniFormat );

    m_qsDownloadAddress     = obPrefFile.value( QString::fromAscii( "PreProcess/Address" ), "" ).toString();
    m_qsProcessFile         = obPrefFile.value( QString::fromAscii( "PreProcess/InfoFile" ), "" ).toString();

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
    if( p_qsText.length() > 0 )
        _progressText( p_qsText );
}
//=================================================================================================
// _progressValue
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_progressValue(int p_nValue)
//-------------------------------------------------------------------------------------------------
{
    ui->progressBar->setValue( p_nValue );
}
//=================================================================================================
// _progressMax
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_progressMax(int p_nValue)
//-------------------------------------------------------------------------------------------------
{
    ui->progressBar->setMaximum( p_nValue );
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
//=================================================================================================
// _progressText
//-------------------------------------------------------------------------------------------------
bool MainWindow::_downloadFile(QString p_qsFileName)
{
    QUrl        url( p_qsFileName );
    QFileInfo   fileInfo(url.path());

    if( fileInfo.fileName().isEmpty() )
        return false;

    QString     fileName = QString("%1\\%2").arg(m_qsCurrentDir).arg( fileInfo.fileName() );

    if( QFile::exists(fileName) )
    {
        QFile::remove(fileName);
    }

    obFile = new QFile(fileName);

    if( !obFile->open(QIODevice::WriteOnly) )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("Unable to save the file\n\n%1\n\n%2.").arg( fileName ).arg( obFile->errorString() ) );
        delete obFile;
        obFile = 0;
        return false;
    }

    QHttp::ConnectionMode mode = url.scheme().toLower() == "https" ? QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp;
    obHttp->setHost(url.host(), mode, url.port() == -1 ? 0 : url.port());

    // http://username:password@example.com/filename.ext
    if (!url.userName().isEmpty())
        obHttp->setUser(url.userName(), url.password());

    m_httpRequestAborted = false;
    QByteArray path = QUrl::toPercentEncoding(url.path(), "!$&'()*+,;=:@/");
    if (path.isEmpty())
        path = "/";
    m_httpGetId = obHttp->get(path, obFile);

    return true;
}
//=================================================================================================
// _progressText
//-------------------------------------------------------------------------------------------------
void MainWindow::_slotHttpRequestFinished(int requestId, bool error)
{
    if (requestId != m_httpGetId) return;

    if (m_httpRequestAborted)
    {
        if (obFile)
        {
            obFile->close();
            obFile->remove();
            delete obFile;
            obFile = 0;
        }
        return;
    }

    if (requestId != m_httpGetId)
        return;

    obFile->close();

    if (error)
    {
        obFile->remove();
        QMessageBox::warning( this, tr("Warning"),
                              tr("Download failed: %1.").arg( obHttp->errorString() ) );
        delete obFile;
        obFile = 0;
        return;
    }

    delete obFile;
    obFile = 0;

// !!!TODO!!! MI VAN HA LETOLTOTTE
    m_nDownload++;

    if( m_nDownload < m_qslDownload.count() )
        _downloadFile( m_qslDownload.at(m_nDownload) );
}
//=================================================================================================
// _progressText
//-------------------------------------------------------------------------------------------------
void MainWindow::_slotReadResponseHeader(const QHttpResponseHeader &responseHeader)
{
    switch (responseHeader.statusCode())
    {
        case 200:                   // Ok
        case 301:                   // Moved Permanently
        case 302:                   // Found
        case 303:                   // See Other
        case 307:                   // Temporary Redirect
            // these are not error conditions
            break;

        default:
            QMessageBox::warning( this, tr("Warning"),
                                  tr("Download failed: %1.").arg( responseHeader.reasonPhrase() ) );
            m_httpRequestAborted = true;
            obHttp->abort();
    }
}
//=================================================================================================
// _progressText
//-------------------------------------------------------------------------------------------------
void MainWindow::_slotUpdateDataReadProgress(int bytesRead, int totalBytes)
{
    if (m_httpRequestAborted)
        return;

    _progressMax( totalBytes );
    _progressValue( bytesRead );
}
//=================================================================================================
// _progressText
//-------------------------------------------------------------------------------------------------
void MainWindow::_slotAuthenticationRequired(const QString &/*hostName*/, quint16, QAuthenticator */*authenticator*/)
{
    /*
    QDialog dlg;
    Ui::Dialog ui;
    ui.setupUi(&dlg);
    dlg.adjustSize();
    ui.siteDescription->setText(tr("%1 at %2").arg(authenticator->realm()).arg(hostName));

    if (dlg.exec() == QDialog::Accepted)
    {
        authenticator->setUser(ui.userEdit->text());
        authenticator->setPassword(ui.passwordEdit->text());
    }
    */
}
//=================================================================================================
// _progressText
//-------------------------------------------------------------------------------------------------
#ifndef QT_NO_OPENSSL
void MainWindow::_slotSslErrors(const QList<QSslError> &/*errors*/)
{
/*
    QString errorString;
    foreach( const QSslError &error, errors )
    {
        if (!errorString.isEmpty())
            errorString += ", ";
        errorString += error.errorString();
    }

    if( QMessageBox::warning( this, tr("Warning"),
                             tr("One or more SSL errors has occurred: %1").arg(errorString),
                             QMessageBox::Ignore | QMessageBox::Abort) == QMessageBox::Ignore)
    {
*/
        obHttp->ignoreSslErrors();
//    }
}
#endif
//=================================================================================================
// _log
//-------------------------------------------------------------------------------------------------
void MainWindow::_log(QString p_qsLogMessage)
{
    QFile   qfLog( QString("log_%1.log").arg( QDate::currentDate().toString("yyyyMMdd") ) );

    if( qfLog.open(QIODevice::Append) )
    {
        qfLog.write( p_qsLogMessage.toStdString().c_str() );
        qfLog.close();
    }
}
//=================================================================================================
