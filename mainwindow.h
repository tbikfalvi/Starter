#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void        processMain();

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

    bool            _checkEnvironment();
    bool            _readSettings();

    void            _progressInit( int p_nMax = 100, QString p_qsText = "" );
    void            _progressStep();
    void            _progressText( QString p_qsText = "" );
};

#endif // MAINWINDOW_H
