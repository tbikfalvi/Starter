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
#include "ui_authenticationdialog.h"

//=================================================================================================

#define CONST_PROCESS_STEP_WAIT_MS  300

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
    m_qsLang            = "";
    m_qsCurrentDir      = QDir::currentPath();
    m_qsProcessFile     = "";
    m_nDownload         = 0;
    m_qsVersion         = "1.0.0";
    m_nVersion          = 0;

    m_teProcessStep     = ST_CHECK_ENVIRONMENT;

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
    m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );

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
    if( m_teProcessStep != ST_EXIT )
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
    //---------------------------------------------------------------------
    // Kill timer to be sure the defined process step executed only once
    killTimer( m_nTimer );
    m_nTimer = 0;

    //---------------------------------------------------------------------
    // Execute defined process step
    switch( m_teProcessStep )
    {
        //---------------------------------------------------------------------
        // Check environment
        case ST_CHECK_ENVIRONMENT:
        {
            _log( tr("Checking environment ...\n") );
            if( _checkEnvironment() )
            {
                _log( tr("Environment check SUCCEEDED\n") );
                if( m_qsDownloadAddress.length() > 0 && m_qsProcessFile.length() > 0 )
                {
                    // The download path and the file name set
                    m_teProcessStep = ST_GET_INFO_FILE;
                }
                else
                {
                    // The download path not set the file should be already in download
                    m_teProcessStep = ST_READ_INFO_FILE;
                }
            }
            else
            {
                m_teProcessStep = ST_EXIT;
            }
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_GET_INFO_FILE:
        {
            _log( tr("Download process XML file\n") );
            _downloadProcessXML();
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_READ_INFO_FILE:
        {
            _log( tr("Read process XML file\n") );
            _readProcessXML();
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_PROCESS_INFO_FILE:
        {
            _log( "Process info file" );
            _progressText( tr("Processing process XML file") );
            _parseProcessXML();
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_DOWNLOAD_FILES:
        {
            _progressText( tr("Downloading files ...") );
            m_teProcessStep = ST_UNCOMPRESS_FILES;
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_UNCOMPRESS_FILES:
        {
            _progressText( tr("Uncompressing downloaded files ...") );
            m_teProcessStep = ST_BACKUP_FILES;
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_BACKUP_FILES:
        {
            _progressText( tr("Backup selected files ...") );
            m_teProcessStep = ST_COPY_FILES;
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_COPY_FILES:
        {
            _progressText( tr("Updating files ...") );
            m_teProcessStep = ST_EXECUTE_APPS;
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_EXECUTE_APPS:
        {
            _progressText( tr("Executing additional processes ...") );
            m_teProcessStep = ST_EXIT;
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_EXIT:
        {
            _progressText( tr("Finising process ...") );
            _log( tr("Process finished, closing application ...\n") );
            close();
            break;
        }
        //---------------------------------------------------------------------
        // This should not be occured
        default:
            _log( tr("!!! Uknown process status !!!") );
    }

    if( m_teProcessStep != ST_EXIT )
    {
        // Call the next process step
        m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
    }
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
// _downloadProcessXML
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_downloadProcessXML()
//-------------------------------------------------------------------------------------------------
{
    _progressInit( 10, tr("Download info file ...") );

    m_qsDownloadAddress.replace( '\\', '/' );
    if( m_qsDownloadAddress.right(1).compare("/") == 0 )
    {
        m_qsDownloadAddress = m_qsDownloadAddress.left(m_qsDownloadAddress.length()-1);
    }

    m_qslDownload << QString( "%1/%2" ).arg( m_qsDownloadAddress ).arg( m_qsProcessFile );
    m_nDownload = 0;

    _downloadFile( m_qslDownload.at(m_nDownload) );
}
//=================================================================================================
// _readProcessXML
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_readProcessXML()
//-------------------------------------------------------------------------------------------------
{
    _progressInit( 10, tr("Read info file ...") );

    QString qsFileName = QString("%1\\download\\%2").arg( m_qsCurrentDir ).arg( m_qsProcessFile );
    _log( QString("Filename:\n%1\n").arg(qsFileName) );
    QFile   qfFile( qsFileName );

    if( !qfFile.exists() )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("The following file is missing:\n"
                                 "%1\n\n"
                                 "Please create the file and fulfill it with proper data.\n"
                                 "For more information about process file, check the manual\n"
                                 "or contact the application provider.").arg( qsFileName ) );
        m_teProcessStep = ST_EXIT;
        m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
        return;
    }

    _progressStep();
    _log( "Open file\n" );

    if( !qfFile.open(QIODevice::ReadOnly) )
    {
        QMessageBox::warning( this, tr("Warning"), tr("Unable to read the following file."
                                                      "%1").arg( qsFileName ) );
        m_teProcessStep = ST_EXIT;
        m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
        return;
    }

    QString      qsErrorMsg  = "";
    int          inErrorLine = 0;

    _log( "Read to obProcessDoc\n" );

    qfFile.seek( 0 );
    if( !obProcessDoc->setContent( &qfFile, &qsErrorMsg, &inErrorLine ) )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr( "Error occured during parsing file:\n'%1'\n\nError in line %2: %3" ).arg( qsFileName ).arg( inErrorLine ).arg( qsErrorMsg ) );
        qfFile.close();
        m_teProcessStep = ST_EXIT;
        m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
        return;
    }
    qfFile.close();

    _log( "Start to process\n" );
    _progressStep();

    m_teProcessStep = ST_PROCESS_INFO_FILE;
    m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );

    return;
}
//=================================================================================================
// _readProcessXML
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_parseProcessXML()
//-------------------------------------------------------------------------------------------------
{
    _progressInit( 10, tr("Parse info file ...") );

    QDomElement     docRoot     = obProcessDoc->documentElement();
    QDomNodeList    obVersions  = docRoot.elementsByTagName( "version" );

    _log( QString("number of version: %1").arg(obVersions.count()) );

    for( int i=0; i<obVersions.count(); i++ )
    {
        m_qslVersions << obVersions.at(i).toElement().attribute("number");
    }

    m_teProcessStep = ST_DOWNLOAD_FILES;
    m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
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

    m_qsVersion             = obPrefFile.value( QString::fromAscii( "PreProcess/Version" ), "1.0.0" ).toString();
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
    _progressText( tr("Downloading %1").arg( p_qsFileName ) );

    QUrl        url( p_qsFileName );
    QFileInfo   fileInfo(url.path());

    if( fileInfo.fileName().isEmpty() )
        return false;

    QString     fileName = QString("%1\\download\\%2").arg(m_qsCurrentDir).arg( fileInfo.fileName() );

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

    if( m_teProcessStep == ST_GET_INFO_FILE )
    {
        m_teProcessStep = ST_READ_INFO_FILE;
    }
    m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
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
void MainWindow::_slotAuthenticationRequired(const QString &hostName, quint16, QAuthenticator *authenticator)
{
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
