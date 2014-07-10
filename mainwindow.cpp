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

#define CONST_PROCESS_STEP_WAIT_MS  1000

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
    // Initialize the variables
    m_nTimer            = 0;
    m_qsLang            = "";
    m_qsCurrentDir      = QDir::currentPath();
    m_qsProcessFile     = "";
    m_nDownload         = 0;
    m_qsVersion         = "1.0.0";
    m_nVersion          = -1;

    m_teProcessStep     = ST_CHECK_ENVIRONMENT;

    m_qsCurrentDir.replace( '/', '\\' );
    if( m_qsCurrentDir.right(1).compare("\\") == 0 )
    {
        m_qsCurrentDir = m_qsCurrentDir.left(m_qsCurrentDir.length()-1);
    }

    obProcessDoc = new QDomDocument( "StarterProcess" );
    obHttp = new QHttp( this );

    connect(obHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(_slotHttpRequestFinished(int,bool)));
    connect(obHttp, SIGNAL(dataReadProgress(int, int)), this, SLOT(_slotUpdateDataReadProgress(int, int)));
    connect(obHttp, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(_slotReadResponseHeader(const QHttpResponseHeader &)));
    connect(obHttp, SIGNAL(authenticationRequired(const QString &, quint16, QAuthenticator *)), this, SLOT(_slotAuthenticationRequired(const QString &, quint16, QAuthenticator *)));
#ifndef QT_NO_OPENSSL
    connect(obHttp, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(_slotSslErrors(const QList<QSslError> &)));
#endif

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
// setProgressTextColor
//-------------------------------------------------------------------------------------------------
void MainWindow::setProgressTextColor(QString p_qsTextColor)
//-------------------------------------------------------------------------------------------------
{
    ui->lblProgressText->setStyleSheet( QString("QLabel {font: bold; color: #%1;}").arg( p_qsTextColor ) );
    ui->lblProgressText->setAttribute( Qt::WA_TranslucentBackground );
    ui->progressBar->setStyleSheet( QString("QProgressBar {font: bold; color: #%1;}").arg( p_qsTextColor ) );
    ui->progressBar->setAttribute( Qt::WA_TranslucentBackground );
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
            _progressInit( 3, tr("Checking environment ...") );
            if( _checkEnvironment() )
            {
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
        // Download info file
        case ST_GET_INFO_FILE:
        {
            _progressInit( 5, tr("Download info file ...") );
            _downloadProcessXML();
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_READ_INFO_FILE:
        {
            _readProcessXML();
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_PARSE_INFO_FILE:
        {
            _progressText( tr("Parsing info file") );
            _parseProcessXMLVersion();
            m_teProcessStep = ST_PROCESS_INFO_FILE;
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_PROCESS_INFO_FILE:
        {
            _progressText( tr("Processing info file") );
            _executeVersionProcess();
            _progressText( tr("Downloading files ...") );
            _downloadFiles();
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_DOWNLOAD_FILES:
        {
            _progressText( tr("Downloading files ...") );
            _downloadFiles();
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
            m_teProcessStep = ST_UPDATE_VERSION_INFO;
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_UPDATE_VERSION_INFO:
        {
            if( m_nVersion+1 < m_qslVersions.count() )
            {
                _progressText( tr("Jumping to the next process version ...") );
                m_qsVersion = m_qslVersions.at( m_nVersion+1 );

                QSettings  obPrefFile( "settings.ini", QSettings::IniFormat );

                obPrefFile.setValue( QString::fromAscii( "PreProcess/Version" ), m_qsVersion );

                m_teProcessStep = ST_PROCESS_INFO_FILE;
            }
            else
            {
                m_teProcessStep = ST_EXIT;
            }
            break;
        }
        //---------------------------------------------------------------------
        //
        case ST_EXIT:
        {
            _progressStep();
            close();
            break;
        }
        //---------------------------------------------------------------------
        // Any case that dont need action
        default:
            ; // do nothing
    }

    switch( m_teProcessStep )
    {
        case ST_GET_INFO_FILE:
        case ST_READ_INFO_FILE:
        case ST_UNCOMPRESS_FILES:
        case ST_BACKUP_FILES:
        case ST_COPY_FILES:
        case ST_EXECUTE_APPS:
        case ST_UPDATE_VERSION_INFO:
        case ST_PROCESS_INFO_FILE:
        {
            // Call the next process step
            m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
            break;
        }
        case ST_EXIT:
        {
            _progressInit( 1, tr("Finising process ...") );
            m_nTimer = startTimer( 2000 );
            break;
        }
        default:
            ; // do nothing
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
    QFile   qfFile( "settings.ini" );

    //-----------------------------------------------------------------------------------
    // STEP 1 - checking if ini file exists
    //-----------------------------------------------------------------------------------
    if( !qfFile.exists() )
    {
        m_teProcessStep = ST_WAIT;
        QMessageBox::warning( this, tr("Warning"),
                              tr("The 'settings.ini' file is missing.\n\n"
                                 "Please create the file and fulfill it with proper data.\n"
                                 "For more information about settings file, check the manual\n"
                                 "or contact the application provider.") );
        return false;
    }
    if( !_readSettings() )
    {
        m_teProcessStep = ST_WAIT;
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
            m_teProcessStep = ST_WAIT;
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
            m_teProcessStep = ST_WAIT;
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
    m_qsDownloadAddress.replace( '\\', '/' );
    if( m_qsDownloadAddress.right(1).compare("/") == 0 )
    {
        m_qsDownloadAddress = m_qsDownloadAddress.left(m_qsDownloadAddress.length()-1);
    }

    m_qslDownload << QString( "%1/%2" ).arg( m_qsDownloadAddress ).arg( m_qsProcessFile );
    m_nDownload = 0;
    _progressStep();
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
    QFile   qfFile( qsFileName );

    if( !qfFile.exists() )
    {
        m_teProcessStep = ST_WAIT;
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

    if( !qfFile.open(QIODevice::ReadOnly) )
    {
        m_teProcessStep = ST_WAIT;
        QMessageBox::warning( this, tr("Warning"), tr("Unable to read the following file."
                                                      "%1").arg( qsFileName ) );
        m_teProcessStep = ST_EXIT;
        m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
        return;
    }

    QString      qsErrorMsg  = "";
    int          inErrorLine = 0;


    qfFile.seek( 0 );
    if( !obProcessDoc->setContent( &qfFile, &qsErrorMsg, &inErrorLine ) )
    {
        m_teProcessStep = ST_WAIT;
        QMessageBox::warning( this, tr("Warning"),
                              tr( "Error occured during parsing file:\n'%1'\n\nError in line %2: %3" ).arg( qsFileName ).arg( inErrorLine ).arg( qsErrorMsg ) );
        qfFile.close();
        m_teProcessStep = ST_EXIT;
        m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
        return;
    }
    qfFile.close();

    _progressStep();

    m_teProcessStep = ST_PARSE_INFO_FILE;
    m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );

    return;
}
//=================================================================================================
// _parseProcessXMLVersion
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_parseProcessXMLVersion()
//-------------------------------------------------------------------------------------------------
{
    _progressInit( 10, tr("Parse info file ...") );

    QDomElement     docRoot     = obProcessDoc->documentElement();
    QDomNodeList    obVersions  = docRoot.elementsByTagName( "version" );

    _progressMax( obVersions.count() );

    for( int i=0; i<obVersions.count(); i++ )
    {
        m_qslVersions << obVersions.at(i).toElement().attribute("number");
        _progressStep();
    }

    m_nVersion = -1;
}
//=================================================================================================
// _readSettings
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_executeVersionProcess()
//-------------------------------------------------------------------------------------------------
{
    m_nVersion++;

    if( m_nVersion < m_qslVersions.count() )
    {
        if( m_qslVersions.at( m_nVersion ).compare( m_qsVersion ) == 0 )
        {
            _parseProcessXMLVersionDownload();
        }
        else
        {
            _executeVersionProcess();
        }
    }
}
//=================================================================================================
// _parseProcessXMLVersion
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_parseProcessXMLVersionDownload()
//-------------------------------------------------------------------------------------------------
{
    m_qslDownload.clear();
    m_nDownload = -1;

    QDomElement     docRoot = obProcessDoc->documentElement();
    QDomNodeList    obFiles = docRoot.elementsByTagName( "version" )
                              .at( m_nVersion ).toElement().elementsByTagName( "download" )
                              .at( 0 ).toElement().elementsByTagName( "file" );

    for( int i=0; i<obFiles.count(); i++ )
    {
        m_qslDownload << QString( "%1|%2" )
                         .arg( obFiles.at(i).toElement().attribute("path") )
                         .arg( obFiles.at(i).toElement().attribute("uncompress") );
    }
}
//=================================================================================================
// _downloadFiles
//-------------------------------------------------------------------------------------------------
void MainWindow::_downloadFiles()
//-------------------------------------------------------------------------------------------------
{
    m_teProcessStep = ST_WAIT;

    m_nDownload++;

    if( m_nDownload < m_qslDownload.size() )
    {
        _downloadFile( m_qslDownload.at( m_nDownload ).split( "|" ).at( 0 ) );
    }
    else
    {
        m_teProcessStep = ST_UNCOMPRESS_FILES;
    }
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

    _progressStep();

    obFile = new QFile(fileName);

    if( !obFile->open(QIODevice::WriteOnly) )
    {
        teProcessStep teTemp = m_teProcessStep;

        m_teProcessStep = ST_WAIT;
        QMessageBox::warning( this, tr("Warning"),
                              tr("Unable to save the file\n\n%1\n\n%2.").arg( fileName ).arg( obFile->errorString() ) );
        delete obFile;
        obFile = 0;
        m_teProcessStep = teTemp;
        return false;
    }

    _progressStep();

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

    _progressStep();

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
        m_teProcessStep = ST_WAIT;
        obFile->remove();
        QMessageBox::warning( this, tr("Warning"),
                              tr("Download failed: %1.").arg( obHttp->errorString() ) );
        delete obFile;
        obFile = 0;
        m_teProcessStep = ST_EXIT;
        m_nTimer = startTimer( CONST_PROCESS_STEP_WAIT_MS );
        return;
    }

    delete obFile;
    obFile = 0;

    if( m_teProcessStep == ST_GET_INFO_FILE )
    {
        m_teProcessStep = ST_READ_INFO_FILE;
    }
    else
    {
        m_teProcessStep = ST_DOWNLOAD_FILES;
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
            m_teProcessStep = ST_WAIT;
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
