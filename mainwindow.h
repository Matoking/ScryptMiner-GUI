#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QStringList>
#include <QMap>
#include <QSettings>

#include "json.h"

// Log types
#define STARTED 0
#define SHARE_SUCCESS 1
#define SHARE_FAIL 2
#define ERROR 3
#define LONGPOLL 4
#define FATAL_ERROR 5

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool minerActive;

    QNetworkAccessManager *networkManager;

    QProcess *minerProcess;

    QMap<int, double> threadSpeed;

    QTimer *readTimer;
    QTimer *poolTimer;

    int acceptedShares;
    int rejectedShares;

    int roundAcceptedShares;
    int roundRejectedShares;

    int initThreads;

public slots:
    void startPressed();

    QStringList getArgs();

    void startMining();
    void stopMining();

    void resizeElements();

    void updateSpeed();

    void updatePoolData();

    void checkSettings();
    void saveSettings();

    void reportToList(QString, int, QString);

    void minerStarted();

    void minerError(QProcess::ProcessError);
    void minerFinished(int, QProcess::ExitStatus);

    void poolDataLoaded(QNetworkReply*);

    void readProcessOutput();

    QString getTime(QString);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
