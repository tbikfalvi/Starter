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
#include <QProcess>

//=================================================================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_authenticationdialog.h"

//=================================================================================================

#define CONST_PROCESS_STEP_WAIT_MS  500

//=================================================================================================
// MainWindow::MainWindow
//-------------------------------------------------------------------------------------------------
// Constructor of main window
//-------------------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
//-------------------------------------------------------------------------------------------------
{
    //_debug( "Constructor\n" );
    //---------------------------------------------------------------
    // Initialize the variables
    m_qsPathAppHome         = "";                           // Application's home directory
    m_qsProcessFile         = "";                           // Filename of the process info file; defined in settings.ini
    m_qsPostProcessPath     = "";                           // Path of the post process executeable
    m_qsPostProcessFile     = "";                           // Filename of the post process executeable
    m_qsInstallPath         = "";                           // Default path where the installed project can be found
    m_qsDownloadPath        = "";                           // Path where the update packages will be downloaded and can be found
    m_qsBackupPath          = "";                           // Default path where the installed project archived before update
    m_qsVersion             = "1.0.0";                      // Version string of the installed project; defined in settings.ini
                                                            // then modified during the update process base on the
                                                            // sections of process info file
    m_nCountVersion         = -1;                           // Counter for the version update process
    m_nCountDownload        = -1;                           // Counter for the download process
    m_nCountVersionStep     = -1;                           // Counter for the version update process steps
    m_nTimerMs              = CONST_PROCESS_STEP_WAIT_MS;   // Timer for process step set by settings.ini
    m_nTimerId              = 0;                            // Unique identifier for timer
    m_teProcessStep         = ST_CHECK_ENVIRONMENT;         // Current process step identifier
    m_nMaxProcessSteps      = 100;                          // Maximum number of steps executed during the whole process

    //---------------------------------------------------------------
    // Initialize the GUI
    ui->setupUi(this);

    //---------------------------------------------------------------
    // General objects
    obProcessDoc    = new QDomDocument( "StarterProcess" );
    obHttp          = new QHttp( this );

    //_debug( "Connect http\n" );
    // Set Http object preferences
    connect(obHttp, SIGNAL(requestFinished(int, bool)), this, SLOT(_slotHttpRequestFinished(int,bool)));
    connect(obHttp, SIGNAL(dataReadProgress(int, int)), this, SLOT(_slotUpdateDataReadProgress(int, int)));
    connect(obHttp, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(_slotReadResponseHeader(const QHttpResponseHeader &)));
    connect(obHttp, SIGNAL(authenticationRequired(const QString &, quint16, QAuthenticator *)), this, SLOT(_slotAuthenticationRequired(const QString &, quint16, QAuthenticator *)));
#ifndef QT_NO_OPENSSL
    connect(obHttp, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(_slotSslErrors(const QList<QSslError> &)));
#endif
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
// MainWindow::init
//-------------------------------------------------------------------------------------------------
// Initialization of variables and the application
//-------------------------------------------------------------------------------------------------
void MainWindow::init()
{
    //_debug( "init\n" );
    //---------------------------------------------------------------
    // Initialize the GUI

    ui->lblProgressText->setText( "" );

    ui->progressBar->setVisible( false );
    ui->progressBar->setMinimum( 0 );
    ui->progressBar->setMaximum( 100 );
    ui->progressBar->setValue( 0 );

    ui->progressBarMain->setVisible( false );
    ui->progressBarMain->setMinimum( 0 );
    ui->progressBarMain->setMaximum( m_nMaxProcessSteps );
    ui->progressBarMain->setValue( 0 );

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    QSettings   obPrefFile( "settings.ini", QSettings::IniFormat );
    QString     qsFile;

    //---------------------------------------------------------------
    // Load background image
    qsFile = obPrefFile.value( QString::fromAscii( "Settings/Background" ), "" ).toString();
    QFile qfFileBackground( QString("%1/%2").arg(m_qsPathAppHome).arg(qsFile) );

    if( qfFileBackground.exists() )
    {
        ui->frmMain->setStyleSheet( QString( "QFrame { border-image: url(%1); }" ).arg(qsFile) );
    }

    //---------------------------------------------------------------
    // Load logo image
    qsFile = obPrefFile.value( QString::fromAscii( "Settings/Logo" ), "" ).toString();
    QString qsPosition = obPrefFile.value( QString::fromAscii( "Settings/LogoPosition" ), "" ).toString();
    QFile   qfFileLogo( QString("%1/%2").arg(m_qsPathAppHome).arg(qsFile) );

    // Disable first logo controls and only enable them if logo exists
    ui->lblLogo->setVisible( false );

    if( qfFileLogo.exists() )
    {
        _debug( QString("Logo position: (%1) - ").arg(qsPosition) );
        ui->lblLogo->setVisible( true );
        ui->lblLogo->setPixmap( QPixmap( qsFile ) );
        ui->lblLogo->setAttribute( Qt::WA_TranslucentBackground );

        if( qsPosition.contains( "left" ) )
        {
            ui->spacerLeft->changeSize( 0, 20, QSizePolicy::Fixed );
            _log( "left " );
        }
        else if( qsPosition.contains( "right" ) )
        {
            ui->spacerRight->changeSize( 0, 20, QSizePolicy::Fixed );
            _log( "right " );
        }
        else
        {
            _log( "center " );
        }

        if( qsPosition.contains( "top" ) )
        {
            ui->spacerTop->changeSize( 20, 0, QSizePolicy::Fixed );
            _log( "top\n" );
        }
        else if( qsPosition.contains( "bottom" ) )
        {
            ui->spacerBottom->changeSize( 20, 0, QSizePolicy::Fixed );
            _log( "bottom\n" );
        }
        else
        {
            _log( "middle\n" );
        }
    }

    _log( "----------------------------------------------------------------------\n" );
    _log( QString("%1 ").arg( QDateTime::currentDateTime().toString( "hh:mm:ss" ) ) );
    _progressInit( 1, tr("Starting process ...") );
    ui->progressBar->setVisible( true );
    ui->progressBarMain->setVisible( true );

    //_debug( "start timer\n" );
    //---------------------------------------------------------------
    // Start the application process with the timer
    m_nTimerId = startTimer( m_nTimerMs );
}

//=================================================================================================
// setProgressTextColor
//-------------------------------------------------------------------------------------------------
void MainWindow::setProgressTextColor(QString p_qsTextColor)
//-------------------------------------------------------------------------------------------------
{
    //_debug( "settextcolor" );
    //_log( QString(": %1\n").arg( QString("QLabel {font: bold; color: #%1;}").arg( p_qsTextColor ) ) );
    ui->lblProgressText->setStyleSheet( QString("QLabel {font: bold; color: #%1;}").arg( p_qsTextColor ) );
    ui->lblProgressText->setAttribute( Qt::WA_TranslucentBackground );
    ui->progressBar->setStyleSheet( QString("QProgressBar {font: bold; color: #%1;}").arg( p_qsTextColor ) );
    ui->progressBar->setAttribute( Qt::WA_TranslucentBackground );
    ui->progressBarMain->setStyleSheet( QString("QProgressBar {font: bold; color: #%1;}").arg( p_qsTextColor ) );
    ui->progressBarMain->setAttribute( Qt::WA_TranslucentBackground );
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
    killTimer( m_nTimerId );
    m_nTimerId = 0;

    //---------------------------------------------------------------------
    // Execute defined process step
    switch( m_teProcessStep )
    {
        //---------------------------------------------------------------------
        // Check environment
        case ST_CHECK_ENVIRONMENT:
        {
            m_teProcessStep = ST_SKIP;
            _progressInit( 3, tr("Initialization ...") );
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
            m_teProcessStep = ST_DOWNLOAD_INFO_FILE;
            _progressValue( 0 );
            _progressMax( 5 );
            _downloadProcessXML();
            _progressStep();
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_READ_INFO_FILE:
        {
            m_teProcessStep = ST_SKIP;
            _progressValue( 0 );
            _progressMax( 3 );
            if( _readProcessXML() )
            {
                m_teProcessStep = ST_PARSE_INFO_FILE;
            }
            else
            {
                m_teProcessStep = ST_EXIT;
            }
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_PARSE_INFO_FILE:
        {
            m_teProcessStep = ST_SKIP;
            _parseProcessXMLGetVersions();
            m_teProcessStep = ST_PROCESS_VERSION;
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_PROCESS_VERSION:
        {
            _progressText( tr("Processing info file ...") );
            m_nCountVersion++;
            if( m_nCountVersion < m_qslVersions.count() )
            {
                QString qsCurrentVersion = m_qslVersions.at( m_nCountVersion ).split("|").at(0);

                _debug( QString("vCurrent: %1  vStep: %2\n").arg( m_qsVersion ).arg( qsCurrentVersion ) );
                if( qsCurrentVersion.compare( m_qsVersion ) == 0 )
                {
                    m_teProcessStep = ST_PARSE_VERSION_STEP;
                }
                else
                {
                    m_teProcessStep = ST_PROCESS_VERSION;
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
        case ST_PARSE_VERSION_STEP:
        {
            _progressText( tr("Parsing version steps ...") );
            _parseProcessXMLGetVersionSteps();
            m_teProcessStep = ST_PROCESS_VERSION_STEP;
        }

        //---------------------------------------------------------------------
        //
        case ST_PROCESS_VERSION_STEP:
        {
            _progressText( tr("Execute next version step ...") );
            _debug( tr("Version step: %1\n").arg( m_nCountVersionStep+1 ) );
            m_nCountVersionStep++;
            if( m_nCountVersionStep < obProcessVersionSteps.count() )
            {
                m_teProcessStep = ST_SKIP;
                _parseProcessXMLGetVersionStep();
            }
            else
            {
                m_teProcessStep = ST_UPDATE_VERSION_INFO;
            }
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_DOWNLOAD_FILES:
        {
            _downloadFiles();
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_UNCOMPRESS_FILES:
        {
            _progressText( tr("Uncompressing files ...") );
            _uncompressFiles();
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_BACKUP_FILES:
        {
            _progressText( tr("Archiving defined files ...") );
            _backupFiles();
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_COPY_FILES:
        {
            _progressText( tr("Updating files ...") );
            _copyFiles();
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_EXECUTE_APPS:
        {
            _progressText( tr("Execute applications ...") );
            _executeApps();
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_UPDATE_VERSION_INFO:
        {
            m_qsVersion = m_qslVersions.at( m_nCountVersion ).split("|").at(1);

            QSettings  obPrefFile( "settings.ini", QSettings::IniFormat );

            obPrefFile.setValue( QString::fromAscii( "PreProcess/Version" ), m_qsVersion );

            if( m_nCountVersion+1 < m_qslVersions.count() )
            {
                _progressText( tr("Jumping to the next process version ...") );

                m_teProcessStep = ST_PROCESS_VERSION;
            }
            else
            {
                m_teProcessStep = ST_EXIT;
            }
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_WAIT_MS:
        {
            m_nCounterWaitMs++;
            _progressStep();
            if( m_nCounterWaitMs > m_nCounterWaitMax )
            {
                m_teProcessStep = ST_PROCESS_VERSION_STEP;
            }
            break;
        }

        //---------------------------------------------------------------------
        //
        case ST_EXIT:
        {
            _progressStep();
            close();
            if( m_qsPostProcessFile.length() > 0 )
            {
                _executeApp( m_qsPostProcessPath, m_qsPostProcessFile, "", true );
            }
            m_teProcessStep = ST_SKIP;
            break;
        }

        //---------------------------------------------------------------------
        // Any case that dont need action
        default:
            ; // do nothing
    }

    if( m_teProcessStep != ST_WAIT_MS )
    {
        _progressMainStep();
    }

    switch( m_teProcessStep )
    {
        case ST_GET_INFO_FILE:
        //   ST_DOWNLOAD_INFO_FILE - no need for next step; httprequestfinished will call
        case ST_READ_INFO_FILE:
        case ST_PARSE_INFO_FILE:
        case ST_PROCESS_VERSION:
        case ST_PARSE_VERSION_STEP:
        case ST_PROCESS_VERSION_STEP:
        //   ST_DOWNLOAD_FILES - no need for next step; httprequestfinished will call
        case ST_DOWNLOAD_FILES: // lehet mégis kell, az indításhoz
        case ST_UNCOMPRESS_FILES:
        case ST_BACKUP_FILES:
        case ST_COPY_FILES:
        case ST_EXECUTE_APPS:
        case ST_UPDATE_VERSION_INFO:
        case ST_WAIT_MS:
        {
            // Call the next process step
            m_nTimerId = startTimer( m_nTimerMs );
            break;
        }
        case ST_EXIT:
        {
            _progressInit( 1, tr("Finising process ...") );
            m_nTimerId = startTimer( 2000 );
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
        m_teProcessStep = ST_SKIP;
        QMessageBox::warning( this, tr("Warning"),
                              tr("The 'settings.ini' file is missing.\n\n"
                                 "Please create the file and fulfill it with proper data.\n"
                                 "For more information about settings file, check the manual\n"
                                 "or contact the application provider.") );
        return false;
    }
    if( !_readSettings() )
    {
        m_teProcessStep = ST_SKIP;
        QMessageBox::warning( this, tr("Warning"),
                              tr("The 'settings.ini' file content is corrupt.\n\n"
                                 "Please check the file and fulfill it with proper data.\n"
                                 "For more information about settings file, check the manual\n"
                                 "or contact the application provider.") );
        return false;
    }

    if( m_qsDownloadPath.length() == 0 )
    {
        m_qsDownloadPath = QString( "%1/download" ).arg( m_qsPathAppHome );
    }

    if( m_qsBackupPath.length() == 0 )
    {
        m_qsBackupPath = QString( "%1/backup" ).arg( m_qsPathAppHome );
    }

    _progressStep();

    //-----------------------------------------------------------------------------------
    // STEP 2 - checking if backup directory exists
    //-----------------------------------------------------------------------------------
    QDir    qdBackup( m_qsBackupPath );

    if( !qdBackup.exists() )
    {
        if( !qdBackup.mkpath( m_qsBackupPath ) )
        {
            m_teProcessStep = ST_SKIP;
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
    QDir    qdDownload( QString("%1/download").arg(m_qsPathAppHome) );

    if( !qdDownload.exists() )
    {
        if( !qdDownload.mkpath( m_qsDownloadPath ) )
        {
            m_teProcessStep = ST_SKIP;
            QMessageBox::warning( this, tr("Warning"),
                                  tr("The 'download' directory not exists and can not be created.\n\n"
                                     "Please create the backup directory in the home directory"
                                     "of this application and restart it again.") );
            return false;
        }
    }

    _progressStep();

    _log( "\nEnvironment settings:\n" );
    _log( QString("Home directory: %1\n").arg( m_qsPathAppHome ) );
    _log( QString("Download directory: %1\n").arg( m_qsDownloadPath ) );
    _log( QString("Backup directory: %1\n").arg( m_qsBackupPath ) );
    _log( QString("Application install directory: %1\n").arg( m_qsInstallPath ) );
    _log( "\n" );

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
    m_nCountDownload = 0;
    _progressStep();
    _downloadFile( m_qslDownload.at(m_nCountDownload) );
}

//=================================================================================================
// _readProcessXML
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
bool MainWindow::_readProcessXML()
//-------------------------------------------------------------------------------------------------
{
    QString qsFileName = QString("%1/%2").arg( m_qsDownloadPath ).arg( m_qsProcessFile );
    QFile   qfFile( qsFileName );

    _log( tr("Process file: %1\n").arg( qsFileName ) );

    if( !qfFile.exists() )
    {
        m_teProcessStep = ST_SKIP;
        QMessageBox::warning( this, tr("Warning"),
                              tr("The following file is missing:\n"
                                 "%1\n\n"
                                 "Please create the file and fulfill it with proper data.\n"
                                 "For more information about process file, check the manual\n"
                                 "or contact the application provider.").arg( qsFileName ) );
        return false;
    }

    _progressStep();

    if( !qfFile.open(QIODevice::ReadOnly) )
    {
        m_teProcessStep = ST_SKIP;
        QMessageBox::warning( this, tr("Warning"), tr("Unable to read the following file."
                                                      "%1").arg( qsFileName ) );
        return false;
    }

    QString      qsErrorMsg  = "";
    int          inErrorLine = 0;

    qfFile.seek( 0 );
    if( !obProcessDoc->setContent( &qfFile, &qsErrorMsg, &inErrorLine ) )
    {
        m_teProcessStep = ST_SKIP;
        QMessageBox::warning( this, tr("Warning"),
                              tr( "Error occured during parsing file:\n'%1'\n\nError in line %2: %3" ).arg( qsFileName ).arg( inErrorLine ).arg( qsErrorMsg ) );
        qfFile.close();
        return false;
    }
    qfFile.close();

    _progressStep();

    return true;
}

//=================================================================================================
// _parseProcessXMLGetVersions
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_parseProcessXMLGetVersions()
//-------------------------------------------------------------------------------------------------
{
    QDomElement     docRoot     = obProcessDoc->documentElement();
    QDomNodeList    obVersions  = docRoot.elementsByTagName( "version" );

    _debug( tr( "Number of version steps: %1\n" ).arg( obVersions.count() ) );

    m_nMaxProcessSteps = obVersions.count();

    for( int i=0; i<obVersions.count(); i++ )
    {
        m_qslVersions << QString("%1|%2")
                                .arg( obVersions.at(i).toElement().attribute("current") )
                                .arg( obVersions.at(i).toElement().attribute("next") );

        obProcessVersionSteps   = docRoot.elementsByTagName( "version" ).at( i ).childNodes();

        _debug( tr( "Number of steps for version %1: %2\n" )
                  .arg( obVersions.at(i).toElement().attribute("current") )
                  .arg( obProcessVersionSteps.count() ) );

        for( int j=0; j<obProcessVersionSteps.count(); j++ )
        {
            if( obProcessVersionSteps.at(j).nodeName().compare( "#comment" ) == 0 )
                continue;

            QDomNodeList stepItems = obProcessVersionSteps.at(j).childNodes();

            _debug( tr( "Number of items for step %1: %2\n" )
                      .arg( obProcessVersionSteps.at(j).nodeName() )
                      .arg( stepItems.count() ) );

            m_nMaxProcessSteps += stepItems.count();
        }
    }

    _debug( QString( "Number of steps to execute: %1\n" ).arg( m_nMaxProcessSteps ) );
    m_nCountVersion = -1;
    _progressStep();
    _progressMainMax( m_nMaxProcessSteps + ui->progressBarMain->value() );
}

//=================================================================================================
// _parseProcessXMLGetVersionSteps
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_parseProcessXMLGetVersionSteps()
{
    QDomElement     docRoot = obProcessDoc->documentElement();

    obProcessVersionSteps   = docRoot.elementsByTagName( "version" ).at( m_nCountVersion ).childNodes();
    m_nCountVersionStep     = -1;

    _debug( QString( "Versionsteps count: %1\n" ).arg( obProcessVersionSteps.count() ) );
}

//=================================================================================================
// _parseProcessXMLGetVersionStep
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_parseProcessXMLGetVersionStep()
{
    QString qsVersionStep = obProcessVersionSteps.at( m_nCountVersionStep ).toElement().nodeName();

    _debug( QString( "VersionStep: %1\n" ).arg( qsVersionStep ) );

    if( qsVersionStep.compare( "download" ) == 0 )
    {
        _parseProcessXMLGetVersionDownloads();
        m_teProcessStep = ST_DOWNLOAD_FILES;
    }
    else if( qsVersionStep.compare( "wait" ) == 0 )
    {
        _initWaitProcess();
        m_teProcessStep = ST_WAIT_MS;
    }
    else if( qsVersionStep.compare( "uncompress" ) == 0 )   m_teProcessStep = ST_UNCOMPRESS_FILES;
    else if( qsVersionStep.compare( "backup" ) == 0 )       m_teProcessStep = ST_BACKUP_FILES;
    else if( qsVersionStep.compare( "update" ) == 0 )       m_teProcessStep = ST_COPY_FILES;
    else if( qsVersionStep.compare( "execute" ) == 0 )      m_teProcessStep = ST_EXECUTE_APPS;
}

//=================================================================================================
// _parseProcessXMLGetVersionDownloads
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_parseProcessXMLGetVersionDownloads()
//-------------------------------------------------------------------------------------------------
{
    m_qslDownload.clear();
    m_nCountDownload = -1;

/*    QDomElement     docRoot = obProcessDoc->documentElement();
    QDomNodeList    obFiles = docRoot.elementsByTagName( "version" )
                              .at( m_nCountVersion ).toElement().elementsByTagName( "download" )
                              .at( 0 ).toElement().elementsByTagName( "file" );*/
    QDomNodeList obFiles = obProcessVersionSteps.at( m_nCountVersionStep ).toElement().elementsByTagName( "file" );

    _debug( "download files:\n" );
    for( int i=0; i<obFiles.count(); i++ )
    {
        _log( QString( "%1\n" ).arg( obFiles.at(i).toElement().attribute("path") ) );
        m_qslDownload << obFiles.at(i).toElement().attribute("path");
    }
}

//=================================================================================================
// _initWaitProcess
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_initWaitProcess()
//-------------------------------------------------------------------------------------------------
{
    if( m_nTimerMs == 0 )
        m_nTimerMs = 250;

    m_nCounterWaitMs    = 0;
    int nWaitSeconds    = obProcessVersionSteps.at( m_nCountVersionStep ).toElement().attribute( "seconds" ).toInt();
    m_nCounterWaitMax   = nWaitSeconds * 1000 / m_nTimerMs;
    _progressInit( m_nCounterWaitMax, tr("Wait %1 seconds ...").arg( nWaitSeconds ) );
}

//=================================================================================================
// _downloadFiles
//-------------------------------------------------------------------------------------------------
void MainWindow::_downloadFiles()
//-------------------------------------------------------------------------------------------------
{
    m_teProcessStep = ST_SKIP;

    m_nCountDownload++;

    if( m_nCountDownload < m_qslDownload.size() )
    {
        _downloadFile( m_qslDownload.at( m_nCountDownload ) );
    }
    else
    {
        m_teProcessStep = ST_PROCESS_VERSION_STEP;
    }
}

//=================================================================================================
// _uncompressFiles
//-------------------------------------------------------------------------------------------------
void MainWindow::_uncompressFiles()
//-------------------------------------------------------------------------------------------------
{
    m_teProcessStep = ST_SKIP;

    QDomElement     docRoot = obProcessDoc->documentElement();
    QDomNodeList    obFiles = docRoot.elementsByTagName( "version" )
                              .at( m_nCountVersion ).toElement().elementsByTagName( "uncompress" )
                              .at( 0 ).toElement().elementsByTagName( "file" );

    _log( QString( "Uncompressing %1 files\n" ).arg( obFiles.count() ) );

    _progressMax( obFiles.count() );

    for( int i=0; i < obFiles.count(); i++ )
    {
        QString qsPath  = obFiles.at(i).toElement().attribute("path");
        QString qsFile  = obFiles.at(i).toElement().attribute("name");

        qsPath.replace( "%INSTALL_DIR%", m_qsInstallPath );
        qsPath.replace( "%DOWNLOAD_DIR%", m_qsDownloadPath );
        qsPath.replace( "%BACKUP_DIR%", m_qsBackupPath );
        qsPath.replace( "%CURRENT_DIR%", m_qsPathAppHome );

        if( !_uncompressFile( qsPath, qsFile ) )
        {
            m_teProcessStep = ST_EXIT;
            return;
        }
        _progressStep();
        _progressMainStep();
    }
    m_teProcessStep = ST_PROCESS_VERSION_STEP;
}

//=================================================================================================
// _backupFiles
//-------------------------------------------------------------------------------------------------
void MainWindow::_backupFiles()
//-------------------------------------------------------------------------------------------------
{
    m_teProcessStep = ST_SKIP;

    QDomElement     docRoot = obProcessDoc->documentElement();
    QDomNodeList    obFiles = docRoot.elementsByTagName( "version" )
                              .at( m_nCountVersion ).toElement().elementsByTagName( "backup" )
                              .at( 0 ).toElement().elementsByTagName( "file" );

    _progressMax( obFiles.count() );

    QString qsBackup = QString( "v_%1/%2" ).arg( m_qsVersion.replace(".","_") ).arg( QDateTime::currentDateTime().toString( "yyyyMMddhhmmss" ) );

    for( int i=0; i<obFiles.count(); i++ )
    {
        QString qsPath  = obFiles.at(i).toElement().attribute("path");
        QString qsDir   = obFiles.at(i).toElement().attribute("directory");
        QString qsFile  = obFiles.at(i).toElement().attribute("name");
        QString qsMove  = obFiles.at(i).toElement().attribute("move");

        qsPath.replace( "%INSTALL_DIR%", m_qsInstallPath );
        qsPath.replace( "%DOWNLOAD_DIR%", m_qsDownloadPath );
        qsPath.replace( "%BACKUP_DIR%", m_qsBackupPath );
        qsPath.replace( "%CURRENT_DIR%", m_qsPathAppHome );

        if( !_backupFile( qsBackup, qsPath, qsDir, qsFile, qsMove ) )
        {
            m_teProcessStep = ST_EXIT;
            return;
        }
        _progressStep();
        _progressMainStep();
    }
    m_teProcessStep = ST_PROCESS_VERSION_STEP;
}

//=================================================================================================
// _copyFiles
//-------------------------------------------------------------------------------------------------
void MainWindow::_copyFiles()
//-------------------------------------------------------------------------------------------------
{
    m_teProcessStep = ST_SKIP;

    QDomElement     docRoot = obProcessDoc->documentElement();
    QDomNodeList    obFiles = docRoot.elementsByTagName( "version" )
                              .at( m_nCountVersion ).toElement().elementsByTagName( "update" )
                              .at( 0 ).toElement().elementsByTagName( "file" );

    _progressMax( obFiles.count() );

    for( int i=0; i<obFiles.count(); i++ )
    {
        QString qsSrc   = obFiles.at(i).toElement().attribute("src");
        QString qsDst   = obFiles.at(i).toElement().attribute("dst");

        qsSrc.replace( "%INSTALL_DIR%", m_qsInstallPath );
        qsSrc.replace( "%DOWNLOAD_DIR%", m_qsDownloadPath );
        qsSrc.replace( "%BACKUP_DIR%", m_qsBackupPath );
        qsSrc.replace( "%CURRENT_DIR%", m_qsPathAppHome );
        qsSrc.replace( "\\", "/" );

        qsDst.replace( "%INSTALL_DIR%", m_qsInstallPath );
        qsDst.replace( "%DOWNLOAD_DIR%", m_qsDownloadPath );
        qsDst.replace( "%BACKUP_DIR%", m_qsBackupPath );
        qsDst.replace( "%CURRENT_DIR%", m_qsPathAppHome );
        qsDst.replace( "\\", "/" );

        if( QFile::exists(qsDst) )
        {
            QFile::remove(qsDst);
        }

        if( !QFile::copy( qsSrc, qsDst ) )
        {
            QMessageBox::warning( this, tr("Warning"),
                                  tr("Unable to copy file ...\n\nSource: %1\nDestination: %2").arg( qsSrc ).arg( qsDst ) );
            m_teProcessStep = ST_EXIT;
            return;
        }
        _progressStep();
        _progressMainStep();
    }
    m_teProcessStep = ST_PROCESS_VERSION_STEP;
}

//=================================================================================================
// _executeApps
//-------------------------------------------------------------------------------------------------
void MainWindow::_executeApps()
//-------------------------------------------------------------------------------------------------
{
    m_teProcessStep = ST_SKIP;

    QDomElement     docRoot = obProcessDoc->documentElement();
    QDomNodeList    obApps = docRoot.elementsByTagName( "version" )
                              .at( m_nCountVersion ).toElement().elementsByTagName( "execute" )
                              .at( 0 ).toElement().elementsByTagName( "application" );

    _progressInit( obApps.count(), tr("Executing additional processes ...") );

    for( int i=0; i<obApps.count(); i++ )
    {
        QString qsPath      = obApps.at(i).toElement().attribute("home");
        QString qsApp       = obApps.at(i).toElement().attribute("name");
        QString qsParam     = obApps.at(i).toElement().attribute("params");
        QString qsDetached  = obApps.at(i).toElement().attribute("detached");

        qsPath.replace( "%INSTALL_DIR%", m_qsInstallPath );
        qsPath.replace( "%DOWNLOAD_DIR%", m_qsDownloadPath );
        qsPath.replace( "%BACKUP_DIR%", m_qsBackupPath );
        qsPath.replace( "%CURRENT_DIR%", m_qsPathAppHome );

        qsParam.replace( "%INSTALL_DIR%", m_qsInstallPath );
        qsParam.replace( "%DOWNLOAD_DIR%", m_qsDownloadPath );
        qsParam.replace( "%BACKUP_DIR%", m_qsBackupPath );
        qsParam.replace( "%CURRENT_DIR%", m_qsPathAppHome );

        bool bDetached = false;

        if( qsDetached.compare("true") == 0 )
        {
            bDetached = true;
        }

        if( !_executeApp( qsPath, qsApp, qsParam, bDetached ) )
        {
            m_teProcessStep = ST_EXIT;
            return;
        }
        _progressStep();
        _progressMainStep();
    }
    m_teProcessStep = ST_PROCESS_VERSION_STEP;
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

    m_qsVersion         = obPrefFile.value( QString::fromAscii( "PreProcess/Version" ), "1.0.0" ).toString();
    m_qsDownloadAddress = obPrefFile.value( QString::fromAscii( "PreProcess/Address" ), "" ).toString();
    m_qsProcessFile     = obPrefFile.value( QString::fromAscii( "PreProcess/InfoFile" ), "" ).toString();
    m_qsInstallPath     = obPrefFile.value( QString::fromAscii( "PreProcess/InstallDir" ), "" ).toString();
    m_qsDownloadPath    = obPrefFile.value( QString::fromAscii( "PreProcess/DownloadDir" ), "" ).toString();
    m_qsBackupPath      = obPrefFile.value( QString::fromAscii( "PreProcess/BackupDir" ), "" ).toString();
    m_qsPostProcessPath = obPrefFile.value( QString::fromAscii( "PostProcess/HomeDir" ), "" ).toString();
    m_qsPostProcessFile = obPrefFile.value( QString::fromAscii( "PostProcess/File" ), "" ).toString();

    m_qsInstallPath.replace( "\\", "/" );
    m_qsDownloadPath.replace( "\\", "/" );
    m_qsBackupPath.replace( "\\", "/" );
    m_qsPostProcessPath.replace( "\\", "/" );

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
    if( ui->progressBar->value() < ui->progressBar->maximum() )
    {
        ui->progressBar->setValue( ui->progressBar->value()+1 );
    }
}

//=================================================================================================
// _progressMainValue
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_progressMainValue( int p_nValue )
//-------------------------------------------------------------------------------------------------
{
    ui->progressBarMain->setValue( p_nValue );
}

//=================================================================================================
// _progressMainMax
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_progressMainMax( int p_nValue )
//-------------------------------------------------------------------------------------------------
{
    ui->progressBarMain->setMaximum( p_nValue );
}

//=================================================================================================
// _progressMainStep
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_progressMainStep()
//-------------------------------------------------------------------------------------------------
{
    if( ui->progressBarMain->value() == ui->progressBarMain->maximum() - 1 )
    {
        _progressMainMax( ui->progressBarMain->maximum() + 1 );
    }

    ui->progressBarMain->setValue( ui->progressBarMain->value()+1 );
}

//=================================================================================================
// _progressText
//-------------------------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------------------------
void MainWindow::_progressText(QString p_qsText)
//-------------------------------------------------------------------------------------------------
{
    _log( QString( "%1\n" ).arg( p_qsText ) );
    ui->lblProgressText->setText( p_qsText );
}

//=================================================================================================
// _downloadFile
//-------------------------------------------------------------------------------------------------
bool MainWindow::_downloadFile(QString p_qsFileName)
{
    _progressText( tr("Downloading %1").arg( p_qsFileName ) );

    QUrl        url( p_qsFileName );
    QFileInfo   fileInfo(url.path());

    if( fileInfo.fileName().isEmpty() )
        return false;

    QString     fileName = QString("%1/%2").arg(m_qsDownloadPath).arg( fileInfo.fileName() );

    if( QFile::exists(fileName) )
    {
        QFile::remove(fileName);
    }

    _progressStep();

    obFile = new QFile(fileName);

    if( !obFile->open(QIODevice::WriteOnly) )
    {
        teProcessStep teTemp = m_teProcessStep;

        m_teProcessStep = ST_SKIP;
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
// _uncompressFile
//-------------------------------------------------------------------------------------------------
bool MainWindow::_uncompressFile(QString p_qsPath, QString p_qsFileName)
{
    QUrl        url( p_qsFileName );
    QFileInfo   fileInfo(url.path());

    if( fileInfo.fileName().isEmpty() )
        return false;

    QDir::setCurrent( p_qsPath );

    QString     qsFileName      = QString( "%1/%2" ).arg( p_qsPath ).arg( fileInfo.fileName() );

    qsFileName.replace( "\\", "/" );
    qsFileName.replace( "//", "/" );

    QString     qsProcess       = QString( "\"%1/pkzip.exe\" -extract -directories=relative -overwrite=all \"%2\"" ).arg( m_qsPathAppHome ).arg( qsFileName );

    _log( QString("Executing %1").arg( qsProcess ) );

    QProcess   *qpUncompress    = new QProcess(this);
    int         nRet            = qpUncompress->execute( qsProcess );

    QDir::setCurrent( m_qsPathAppHome );

    if( nRet < 0 )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("Error occured when starting process:\n\n%1\n\nError code: %2\n"
                                 "-2 > Process cannot be started\n"
                                 "-1 > Process crashed").arg(qsProcess).arg(nRet) );
        return false;
    }

    return true;
}

//=================================================================================================
// _backupFile
//-------------------------------------------------------------------------------------------------
bool MainWindow::_backupFile( QString p_qsBackup, QString p_qsPath, QString p_qsDir, QString p_qsFile, QString p_qsMove )
{
    QString qsSrc = QString("%1/").arg( p_qsPath );

    if( p_qsDir.length() > 0 )
    {
        qsSrc.append( p_qsDir );
        qsSrc.append( "/");
    }
    qsSrc.append( p_qsFile );

    QString qsDst = QString( "%1/%2/" ).arg( m_qsBackupPath ).arg( p_qsBackup );

    if( p_qsDir.length() > 0 )
    {
        qsDst.append( p_qsDir );
        qsDst.append( "/");
    }

    qsSrc.replace( "\\", "/" );
    qsDst.replace( "\\", "/" );

    QDir    qdDst;

    qdDst.mkpath( qsDst );
    qsDst.append( p_qsFile );

    _log( QString("%1 -> %2\n").arg(qsSrc).arg(qsDst) );

    bool bRet = QFile::copy( qsSrc, qsDst );

    if( bRet && p_qsMove.compare("yes") == 0 )
    {
        QFile fileSrc;

        fileSrc.setFileName( qsSrc );

        bRet = fileSrc.remove();
    }

    return bRet;
}

//=================================================================================================
// _executeApp
//-------------------------------------------------------------------------------------------------
bool MainWindow::_executeApp( QString p_qsPath, QString p_qsApplication, QString p_qsParameters, bool p_bDetached )
{
    if( !QDir::setCurrent( p_qsPath ) )
    {
        QMessageBox::warning( this, tr("Warning"),
                              tr("Unable to change current directory to\n\n%1").arg( p_qsPath ) );
        return false;
    }

    QString     qsProcess   = QString( "%1 %2" ).arg( p_qsApplication ).arg( p_qsParameters );
    QProcess   *qpExecute   = new QProcess(this);

    if( p_bDetached )
    {
        _log( QString( "Executing process: %1\n" ).arg( qsProcess ) );
        bool bRet = qpExecute->startDetached( qsProcess );

        QDir::setCurrent( m_qsPathAppHome );

        if( !bRet )
        {
            QMessageBox::warning( this, tr("Warning"),
                                  tr("Error occured when starting process:\n\n%1\n\nError code: %2\n"
                                     "0 > The process failed to start.\n"
                                     "1 > The process crashed some time after starting successfully.\n"
                                     "2 > The last waitFor...() function timed out.\n"
                                     "4 > An error occurred when attempting to write to the process.\n"
                                     "3 > An error occurred when attempting to read from the process.\n"
                                     "5 > An unknown error occurred.").arg( qsProcess ).arg( qpExecute->error() ) );
            return false;
        }
    }
    else
    {
        int nRet = qpExecute->execute( qsProcess.replace( "\\", "/" ) );

        QDir::setCurrent( m_qsPathAppHome );

        if( nRet < 0 )
        {
            QMessageBox::warning( this, tr("Warning"),
                                  tr("Error occured when starting process:\n\n%1\n\nError code: %2\n"
                                     "-2 > Process cannot be started\n"
                                     "-1 > Process crashed").arg(qsProcess).arg(nRet) );
            return false;
        }
    }

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
        m_teProcessStep = ST_SKIP;
        obFile->remove();
        QMessageBox::warning( this, tr("Warning"),
                              tr("Error occured during downloading file:\n%1.").arg( obHttp->errorString() ) );
        delete obFile;
        obFile = 0;
        m_teProcessStep = ST_EXIT;
        m_nTimerId = startTimer( m_nTimerMs );
        return;
    }

    delete obFile;
    obFile = 0;

    if( m_teProcessStep == ST_DOWNLOAD_INFO_FILE )
    {
        m_teProcessStep = ST_READ_INFO_FILE;
    }
    else
    {
        m_teProcessStep = ST_DOWNLOAD_FILES;
    }

    _debug( QString("Downloading file finished. Next: %1\n").arg( m_teProcessStep ) );

    _progressMainStep();

    m_nTimerId = startTimer( m_nTimerMs );
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
            m_teProcessStep = ST_SKIP;
            QMessageBox::warning( this, tr("Warning"),
                                  tr("Download failed: %1.").arg( responseHeader.reasonPhrase() ) );
            m_httpRequestAborted = true;
            obHttp->abort();
            m_teProcessStep = ST_EXIT;
            m_nTimerId = startTimer( m_nTimerMs );
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
// _debug
//-------------------------------------------------------------------------------------------------
void MainWindow::_debug(QString p_qsLogMessage)
{
    if( m_bDebugEnabled ) _log( QString( "DEBUG: %1" ).arg( p_qsLogMessage ) );
}

//=================================================================================================
