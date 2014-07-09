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
        ST_READ_INFO_FILE,
        ST_PROCESS_INFO_FILE,
        ST_DOWNLOAD_FILES,
        ST_UNCOMPRESS_FILES,
        ST_BACKUP_FILES,
        ST_COPY_FILES,
        ST_EXECUTE_APPS,
        ST_EXIT
    };

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

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

    int              m_nTimer;
    bool             m_httpRequestAborted;
    int              m_httpGetId;

    QString          m_qsLang;
    QString          m_qsCurrentDir;
    QString          m_qsDownloadAddress;
    QString          m_qsProcessFile;
    QString          m_qsVersion;

    QStringList      m_qslDownload;
    int              m_nDownload;

    QStringList      m_qslVersions;
    int              m_nVersion;

    QDomDocument    *obProcessDoc;
    QHttp           *obHttp;
    QFile           *obFile;

    bool            _readSettings();
    bool            _checkEnvironment();
    void            _downloadProcessXML();
    void            _readProcessXML();
    void            _parseProcessXML();

    void            _progressInit( int p_nMax = 100, QString p_qsText = "" );
    void            _progressValue( int p_nValue );
    void            _progressMax( int p_nValue );
    void            _progressStep();
    void            _progressText( QString p_qsText = "" );

    bool            _downloadFile( QString p_qsFileName );

    void            _log( QString p_qsLogMessage );
};

#endif // MAINWINDOW_H
