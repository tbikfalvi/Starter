#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtXml/QDomDocument>
#include <QtNetwork/QHttp>
#include <QFile>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool            readSettings();
    void            processMain();

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

    int              m_nTimer;
    bool             m_bProcessFinished;
    bool             m_httpRequestAborted;
    int              m_httpGetId;

    QString          m_qsLang;
    QString          m_qsCurrentDir;
    QString          m_qsDownloadAddress;
    QString          m_qsProcessFile;

    QStringList      m_qslDownload;
    int              m_nDownload;

    QDomDocument    *obProcessDoc;
    QHttp           *obHttp;
    QFile           *obFile;

    bool            _checkEnvironment();
    bool            _downloadProcessXML();
    bool            _readProcessXML();

    void            _progressInit( int p_nMax = 100, QString p_qsText = "" );
    void            _progressValue( int p_nValue );
    void            _progressMax( int p_nValue );
    void            _progressStep();
    void            _progressText( QString p_qsText = "" );

    bool            _downloadFile( QString p_qsFileName );

    void            _log( QString p_qsLogMessage );
};

#endif // MAINWINDOW_H
