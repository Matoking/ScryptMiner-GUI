#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "poolparse.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create system tray icon
    trayIcon = new QSystemTrayIcon(this);

    inactiveIcon.addFile(":/icons/inactiveIcon");
    activeIcon.addFile(":/icons/activeIcon");

    trayIcon->setIcon(inactiveIcon);

    this->setWindowIcon(QIcon(":/icons/icon"));

    createTrayActions();

    networkManager = new QNetworkAccessManager(this);

    setFixedSize(400, 460);

    checkSettings();

    minerActive = false;

    minerProcess = new QProcess(this);
    minerProcess->setProcessChannelMode(QProcess::MergedChannels);

    readTimer = new QTimer(this);
    poolTimer = new QTimer(this);

    poolTimer->setInterval(60000);
    poolTimer->setSingleShot(false);
    poolTimer->start();

    acceptedShares = 0;
    rejectedShares = 0;

    roundAcceptedShares = 0;
    roundRejectedShares = 0;

    initThreads = 0;

    outputFile = new QFile("output.txt");

    connect(readTimer, SIGNAL(timeout()), this, SLOT(readProcessOutput()));
    connect(poolTimer, SIGNAL(timeout()), this, SLOT(updatePoolData()));

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

    connect(ui->startButton, SIGNAL(pressed()), this, SLOT(startPressed()));
    connect(ui->updateButton, SIGNAL(pressed()), this, SLOT(updatePoolData()));

    connect(minerProcess, SIGNAL(started()), this, SLOT(minerStarted()));
    connect(minerProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(minerError(QProcess::ProcessError)));
    connect(minerProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(minerFinished(int, QProcess::ExitStatus)));
    connect(minerProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(readProcessOutput()));

    // Check if we got a --automine argument
    if (qApp->arguments().contains("--automine") || qApp->arguments().contains("-m"))
    {
        hideMainWindow();
        startMining();
    }
}

MainWindow::~MainWindow()
{
    if (outputFile->isOpen())
    {
        outputFile->flush();
        outputFile->close();
    }

    minerProcess->kill();

    saveSettings();

    delete ui;
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
        showMainWindow();
    else
        return;
}

void MainWindow::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange)
    {
        if (isMinimized())
            hideMainWindow();
        else
            showMainWindow();
    }
    QMainWindow::changeEvent(e);
}

void MainWindow::createTrayActions()
{
    QMenu *trayMenu = new QMenu();

    QAction *openAction = new QAction("Open", trayMenu);
    QAction *exitAction = new QAction("Exit", trayMenu);

    trayMenu->addAction(openAction);
    trayMenu->addAction(exitAction);

    connect(openAction, SIGNAL(triggered()), this, SLOT(showMainWindow()));
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

    trayIcon->setContextMenu(trayMenu);
}

void MainWindow::resizeElements()
{
    if (minerActive == true)
    {
        ui->output->setFixedHeight(355);
        ui->list->setFixedHeight(355);
        ui->poolData->setFixedHeight(250);
    }
    else if (minerActive == false)
    {
        ui->output->setFixedHeight(145);
        ui->list->setFixedHeight(145);
        ui->poolData->setFixedHeight(80);
    }
}

void MainWindow::showMainWindow()
{
    setParent(NULL, Qt::Window);
    showNormal();
    trayIcon->hide();
}

void MainWindow::hideMainWindow()
{
    hide();
    trayIcon->show();

    // Hiding the window from the taskbar
    QWidget *tmp = new QWidget();
    setParent(tmp, Qt::SubWindow);
}

void MainWindow::startPressed()
{
    if (minerActive == false)
        startMining();
    else
        stopMining();

}

void MainWindow::recordOutput(QString text)
{
    if (!outputFile->isOpen())
    {
        outputFile->open(QIODevice::ReadWrite);
    }

    outputFile->write(text.toAscii());
}

QStringList MainWindow::getArgs()
{
    QStringList args;
    QString url = ui->rpcServerLine->text();
    if (!url.contains("http://"))
        url.prepend("http://");
    qDebug(url.toAscii());
    QString urlLine = QString("%1:%2").arg(url, ui->portLine->text());
    QString userpassLine = QString("%1:%2").arg(ui->usernameLine->text(), ui->passwordLine->text());
    args << "--algo" << "scrypt";
    args << "-s" << ui->scantimeLine->text().toAscii();
    args << "--url" << urlLine.toAscii();
    args << "--userpass" << userpassLine.toAscii();
    args << "--threads" << ui->threadsBox->currentText().toAscii();
    args << "-P";
    args << ui->parametersLine->text().split(" ", QString::SkipEmptyParts);

    return args;
}

void MainWindow::startMining()
{
    ui->startButton->setText("Stop Mining");
    ui->threadsBox->setDisabled(true);
    ui->scantimeLine->setDisabled(true);
    ui->rpcServerLine->setDisabled(true);
    ui->usernameLine->setDisabled(true);
    ui->passwordLine->setDisabled(true);
    ui->portLine->setDisabled(true);
    ui->parametersLine->setDisabled(true);
    minerActive = true;
    ui->tabWidget->move(ui->tabWidget->x(), 52);
    ui->tabWidget->setFixedHeight(380);
    ui->settingsFrame->setVisible(false);
    resizeElements();

    trayIcon->setIcon(activeIcon);
    QStringList args = getArgs();

    threadSpeed.clear();

    acceptedShares = 0;
    rejectedShares = 0;

    roundAcceptedShares = 0;
    roundRejectedShares = 0;

    initThreads = ui->threadsBox->currentText().toInt();

#ifdef WIN32
    QString program = "minerd";
#else
    QString program = "./minerd";
#endif

    minerProcess->start(program,args);
    minerProcess->waitForStarted(-1);

    readTimer->start(500);

    trayIcon->setToolTip("Mining");
}

void MainWindow::stopMining()
{
    ui->startButton->setText("Start Mining");
    minerActive = false;
    ui->threadsBox->setDisabled(false);
    ui->scantimeLine->setDisabled(false);
    ui->rpcServerLine->setDisabled(false);
    ui->usernameLine->setDisabled(false);
    ui->passwordLine->setDisabled(false);
    ui->portLine->setDisabled(false);
    ui->parametersLine->setDisabled(false);
    ui->tabWidget->move(-1,260);
    ui->tabWidget->setFixedHeight(170);
    ui->settingsFrame->setVisible(true);
    resizeElements();

    trayIcon->setIcon(inactiveIcon);
    reportToList("Miner stopped", 0, NULL);
    ui->mineSpeedLabel->setText("N/A");
    ui->shareCount->setText("Accepted: 0 - Rejected: 0");
    minerProcess->kill();
    readTimer->stop();
    trayIcon->setToolTip("Not mining");
}

void MainWindow::saveSettings()
{
    QSettings settings("easyminer.conf", QSettings::IniFormat);

    settings.setValue("threads", ui->threadsBox->currentText());
    settings.setValue("scantime", ui->scantimeLine->text());
    settings.setValue("url", ui->rpcServerLine->text());
    settings.setValue("username", ui->usernameLine->text());
    settings.setValue("password", ui->passwordLine->text());
    settings.setValue("port", ui->portLine->text());
    settings.setValue("miningPoolIndex", ui->poolBox->currentIndex());
    settings.setValue("poolApiKey", ui->apiKeyLine->text());
    settings.setValue("sharePopup", ui->shareBox->isChecked());
    settings.setValue("minerRestart", ui->restartBox->isChecked());
    settings.setValue("saveOutput", ui->saveOutputBox->isChecked());

    settings.sync();
}

void MainWindow::checkSettings()
{
    QSettings settings("easyminer.conf", QSettings::IniFormat);
    if (settings.value("threads").isValid())
    {
        int threads = settings.value("threads").toInt();
        threads--;
        ui->threadsBox->setCurrentIndex(threads);
    }

    if (settings.value("scantime").isValid())
        ui->scantimeLine->setText(settings.value("scantime").toString());
    else
        ui->scantimeLine->setText("15");

    if (settings.value("url").isValid())
        ui->rpcServerLine->setText(settings.value("url").toString());
    if (settings.value("username").isValid())
        ui->usernameLine->setText(settings.value("username").toString());
    if (settings.value("password").isValid())
        ui->passwordLine->setText(settings.value("password").toString());

    if (settings.value("port").isValid())
        ui->portLine->setText(settings.value("port").toString());
    else
        ui->portLine->setText("9332");

    if (settings.value("miningPoolIndex").isValid())
        ui->poolBox->setCurrentIndex(settings.value("miningPoolIndex").toInt());
    if (settings.value("poolApiKey").isValid())
        ui->apiKeyLine->setText(settings.value("poolApiKey").toString());

    if (settings.value("sharePopup").isValid())
        ui->shareBox->setChecked(settings.value("sharePopup").toBool());
    if (settings.value("minerRestart").isValid())
        ui->restartBox->setChecked(settings.value("minerRestart").toBool());
    if (settings.value("saveOutput").isValid())
        ui->saveOutputBox->setChecked(settings.value("saveOutput").toBool());
}

void MainWindow::readProcessOutput()
{
    QByteArray output;

    minerProcess->reset();

    output = minerProcess->readAll();

    QString outputString(output);

    if (!outputString.isEmpty())
    {
        QStringList list = outputString.split("\n");
        int i;
        for (i=0; i<list.size(); i++)
        {
            QString line = list.at(i);

            // Ignore protocol dump
            if (!line.startsWith("[") || line.contains("JSON protocol") || line.contains("HTTP hdr"))
                continue;

            if (line.contains("Long-polling activated for"))
                reportToList("LONGPOLL activated", LONGPOLL, getTime(line));

            if (line.contains("(yay!!!)"))
                reportToList("Share accepted", SHARE_SUCCESS, getTime(line));

            else if (line.contains("(booooo)"))
                reportToList("Share rejected", SHARE_FAIL, getTime(line));

            else if (line.contains("LONGPOLL detected new block"))
                reportToList("LONGPOLL detected a new block", LONGPOLL, getTime(line));

            else if (line.contains("Supported options:"))
                reportToList("Miner didn't start properly. Try checking your settings.", FATAL_ERROR, NULL);

            else if (line.contains("The requested URL returned error: 403"))
                reportToList("Couldn't connect. Try checking your username and password.", ERROR, NULL);
            else if (line.contains("workio thread dead, exiting."))
            {
                reportToList("Miner exited. Restarting automatically.", ERROR, NULL);
                stopMining();
                startMining();
            }

            else if (line.contains("thread ") && line.contains("khash/s"))
            {
                QString threadIDstr = line.at(line.indexOf("thread ")+7);
                int threadID = threadIDstr.toInt();

                int threadSpeedindx = line.indexOf(",");
                QString threadSpeedstr = line.mid(threadSpeedindx);
                threadSpeedstr.chop(10);
                threadSpeedstr.remove(", ");
                threadSpeedstr.remove(" ");
                threadSpeedstr.remove('\n');
                double speed=0;
                speed = threadSpeedstr.toDouble();
                qDebug(threadSpeedstr.toAscii());

                threadSpeed[threadID] = speed;

                updateSpeed();
            }

            if (line.isEmpty() == false)
            {
                ui->output->append(QString("%1").arg(line));
                if (ui->saveOutputBox->isChecked())
                    recordOutput(line);
            }
        }

        if (ui->output->toPlainText().length() > 4000)
        {
            QString text = ui->output->toPlainText();
            text.remove(0, text.length() - 4000);
            ui->output->setText(text);
        }
    }
}


void MainWindow::minerError(QProcess::ProcessError error)
{
    QString errorMessage;
    switch(error)
    {
        case QProcess::Crashed:
            errorMessage = "Miner killed";
            break;

        case QProcess::FailedToStart:
            errorMessage = "Miner failed to start";
            break;

        default:
            errorMessage = "Unknown error";
            break;
    }

    reportToList(errorMessage, ERROR, NULL);

    trayIcon->showMessage("Miner encountered an error", errorMessage);

}

void MainWindow::minerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString exitMessage;
    switch(exitStatus)
    {
        case QProcess::NormalExit:
            exitMessage = "Miner exited normally";
            break;

        case QProcess::CrashExit:
            exitMessage = "Miner exited.";
            if (ui->restartBox->isChecked())
                startMining();
            break;

        default:
            exitMessage = "Miner exited abnormally.";
            if (ui->restartBox->isChecked())
                startMining();
            break;
    }

    if (exitStatus == QProcess::NormalExit)
        stopMining();
    trayIcon->showMessage("Miner stopped", exitMessage);
}

void MainWindow::minerStarted()
{
    QString argString = "";
    QStringList args = getArgs();

    int i;
    for (i=0; i<=args.length()-1; i++)
    {
        argString.append(args[i]);
        argString.append(" ");
    }
    reportToList(QString("Miner started. It usually takes a minute until the program starts reporting information. Parameters: %1").arg(argString), STARTED, NULL);
}

void MainWindow::updateSpeed()
{
    double totalSpeed=0;
    int totalThreads=0;

    QMapIterator<int, double> iter(threadSpeed);
    while(iter.hasNext())
    {
        iter.next();
        totalSpeed += iter.value();
        totalThreads++;
    }

    // If all threads haven't reported the hash speed yet, make an assumption
    if (totalThreads != initThreads)
    {
        totalSpeed = (totalSpeed/totalThreads)*initThreads;
    }

    QString speedString = QString("%1").arg(totalSpeed);
    QString threadsString = QString("%1").arg(initThreads);

    QString acceptedString = QString("%1").arg(acceptedShares);
    QString rejectedString = QString("%1").arg(rejectedShares);

    QString roundAcceptedString = QString("%1").arg(roundAcceptedShares);
    QString roundRejectedString = QString("%1").arg(roundRejectedShares);

    if (totalThreads == initThreads)
        ui->mineSpeedLabel->setText(QString("%1 khash/sec - %2 thread(s)").arg(speedString, threadsString));
    else
        ui->mineSpeedLabel->setText(QString("~%1 khash/sec - %2 thread(s)").arg(speedString, threadsString));

    ui->shareCount->setText(QString("Accepted: %1 (%3) - Rejected: %2 (%4)").arg(acceptedString, rejectedString, roundAcceptedString, roundRejectedString));

    QString tooltipString = QString("%1 kh/sec -").arg(totalSpeed);
    tooltipString.append(QString("A:%1 - R:%2").arg(acceptedString,rejectedString));

    trayIcon->setToolTip(tooltipString);
}

void MainWindow::updatePoolData()
{
    QString url = PoolParse::getURL(ui->poolBox->currentText(), ui->apiKeyLine->text());

    if (ui->apiKeyLine->text().isEmpty() == false)
    {
        networkManager->get(QNetworkRequest(QUrl(url)));

        connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(poolDataLoaded(QNetworkReply*)));
    }
}

void MainWindow::poolDataLoaded(QNetworkReply *data)
{
    QString poolDataString;

    QByteArray replyBytes = data->readAll();
    QString replyString = QString::fromUtf8(replyBytes);

    QVariantMap replyMap;
    bool parseSuccess;

    replyMap = Json::parse(replyString, parseSuccess).toMap();

    if (parseSuccess == true)
    {
        poolDataString = PoolParse::parseData(ui->poolBox->currentText(), replyMap);
    }
    else
    {
        poolDataString = "Couldn't update mining pool data.";
    }

    ui->poolData->setText(poolDataString);

    disconnect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(poolDataLoaded(QNetworkReply*)));

}

void MainWindow::reportToList(QString msg, int type, QString time)
{
    QString message;
    if (time == NULL)
        message = QString("[%1] - %2").arg(QTime::currentTime().toString(), msg);
    else
        message = QString("[%1] - %2").arg(time, msg);

    switch(type)
    {
        case SHARE_SUCCESS:
            acceptedShares++;
            if (ui->shareBox->isChecked())
                trayIcon->showMessage("Share found", QString("[%1] Accepted share").arg(QTime::currentTime().toString()), QSystemTrayIcon::Information, 5000);
            roundAcceptedShares++;
            updateSpeed();
            break;

        case SHARE_FAIL:
            rejectedShares++;
            roundRejectedShares++;
            updateSpeed();
            break;

        case FATAL_ERROR:
            startPressed();
            break;

        case LONGPOLL:
            roundAcceptedShares = 0;
            roundRejectedShares = 0;
            break;

        default:
            break;
    }

    ui->list->addItem(message);
    ui->list->scrollToBottom();
}

// Function for fetching the time
QString MainWindow::getTime(QString time)
{
    if (time.contains("["))
    {
        time.resize(21);
        time.remove("[");
        time.remove("]");
        time.remove(0,11);

        return time;
    }
    else
        return NULL;
}
