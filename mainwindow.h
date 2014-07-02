#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtXml/QDomDocument>

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

protected:
    void timerEvent( QTimerEvent *p_poEvent );
    void closeEvent( QCloseEvent *p_poEvent );

private:
    Ui::MainWindow  *ui;

    int              m_nTimer;
    bool             m_bProcessFinished;

    QString          m_qsLang;
    QString          m_qsCurrentDir;

    QDomDocument    *obProcessDoc;

    bool            _checkEnvironment();
    bool            _readProcessXML();

    void            _progressInit( int p_nMax = 100, QString p_qsText = "" );
    void            _progressStep();
    void            _progressText( QString p_qsText = "" );
};

#endif // MAINWINDOW_H
