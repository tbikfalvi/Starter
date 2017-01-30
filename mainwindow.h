#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDomDocument>
#include <QHttp>
#include <QFile>
#include <QAuthenticator>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum teProcessStep
    {
        ST_CHECK_ENVIRONMENT = 0,
        ST_GET_INFO_FILE,
        ST_DOWNLOAD_INFO_FILE,
        ST_READ_INFO_FILE,
        ST_PARSE_INFO_FILE,
        ST_PROCESS_VERSION,
        ST_PARSE_VERSION_STEP,
        ST_PROCESS_VERSION_STEP,
        ST_DOWNLOAD_FILES,
        ST_UNCOMPRESS_FILES,
        ST_BACKUP_FILES,
        ST_COPY_FILES,
        ST_EXECUTE_APPS,
        ST_WAIT_MS,
        ST_CONFIRM,
        ST_UPDATE_VERSION_INFO,
        ST_EXIT,
        ST_SKIP
    };

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void setProgressTextColor( QString p_qsTextColor );

    void setTimerIntervall( int p_nTimerMs )            { m_nTimerMs        = p_nTimerMs;       }
    void setAppHomeDirectory( QString p_qsPathAppHome ) { m_qsPathAppHome   = p_qsPathAppHome;  }

    void init();

private slots:
    void on_pbExit_clicked();
    void _slotHttpRequestFinished(int requestId, bool error);
    void _slotUpdateDataReadProgress(int bytesRead, int totalBytes);
    void _slotReadResponseHeader(const QHttpResponseHeader &responseHeader);
    void _slotAuthenticationRequired(const QString &, quint16, QAuthenticator *);
#ifndef QT_NO_OPENSSL
    void _slotSslErrors(const QList<QSslError> &errors);
#endif

protected:
    void timerEvent( QTimerEvent *p_poEvent );
    void closeEvent( QCloseEvent *p_poEvent );

private:
    Ui::MainWindow  *ui;

    teProcessStep    m_teProcessStep;

    int              m_nTimerId;
    bool             m_httpRequestAborted;
    int              m_httpGetId;

    QString          m_qsPathAppHome;
    QString          m_qsCurrentDir;
    QString          m_qsDownloadAddress;
    QString          m_qsProcessFile;
    QString          m_qsVersion;
    int              m_nTimerMs;
    QString          m_qsPostProcessPath;
    QString          m_qsPostProcessFile;
    QString          m_qsInstallPath;
    QString          m_qsDownloadPath;
    QString          m_qsBackupPath;

    QStringList      m_qslDownload;
    int              m_nDownload;

    QStringList      m_qslVersions;
    int              m_nCountVersion;

    int              m_nCountDownload;
    int              m_nCountUncompress;
    int              m_nCountBackup;
    int              m_nCountUpdate;
    int              m_nCountExecute;
    int              m_nCountVersionStep;

    QDomDocument    *obProcessDoc;
    QDomNodeList     obProcessVersionSteps;
    QHttp           *obHttp;
    QFile           *obFile;

    bool            _readSettings();
    bool            _checkEnvironment();
    void            _downloadProcessXML();
    bool            _readProcessXML();
    void            _parseProcessXMLGetVersions();
    void            _parseProcessXMLGetVersionSteps();
    void            _parseProcessXMLGetVersionStep();
    void            _parseProcessXMLGetVersionDownloads();
    void            _downloadFiles();
    void            _uncompressFiles();
    void            _backupFiles();
    void            _copyFiles();
    void            _executeApps();

    void            _progressInit( int p_nMax = 100, QString p_qsText = "" );
    void            _progressValue( int p_nValue );
    void            _progressMax( int p_nValue );
    void            _progressStep();
    void            _progressText( QString p_qsText = "" );

    bool            _downloadFile( QString p_qsFileName );
    bool            _uncompressFile( QString p_qsPath, QString p_qsFileName );
    bool            _backupFile( QString p_qsBackup, QString p_qsPath, QString p_qsDir, QString p_qsFile, QString p_qsMove );
    bool            _executeApp( QString p_qsPath, QString p_qsApplication, QString p_qsParameters, bool p_bDetached = false );

    void            _log( QString p_qsLogMessage );
};

#endif // MAINWINDOW_H
