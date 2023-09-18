#include <QMenuBar>
#include <QGuiApplication>
#include <QScreen>
#include <QDesktopServices>
#include "mainwindow.h"

static ToolRealTimeLog *g_toolRealTimeLog = nullptr;

void realTimeLog(QString log)
{
    if (g_toolRealTimeLog) {
        g_toolRealTimeLog->realtimeLog(QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss.zzz ") + log);
        qDebug() << QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss.zzz ") + log;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    msgView = new MessageView();
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect rect = screen->geometry();

    this->setStyleSheet(".MainWindow{background-color: #FFFFFF;}");

    toolCharHexTransform = new ToolCharHexTransform();
    g_toolRealTimeLog = toolRealTimeLog = new ToolRealTimeLog();

    setCentralWidget(uds.UDSMainWidget());
    QMenuBar *menuBr = menuBar();
    QMenu *fileMenu = menuBr->addMenu("文件(F)");
    QMenu *configMenu = menuBr->addMenu("配置(C)");
    QMenu *toolMenu = menuBr->addMenu("工具(T)");
    QMenu *aboutMenu = menuBr->addMenu("关于(A)");
    setMenuBar(menuBr);

    configMenu->addAction("暂时没有做啥");

    QDockWidget *messageDock = new QDockWidget("消息", this);
    QWidget* msglTitleBar = messageDock->titleBarWidget();
    QLabel *msgemtryLabel = new QLabel();
    messageDock->setTitleBarWidget(msgemtryLabel);
    delete msglTitleBar;
    messageDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    messageDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, messageDock);
    messageDock->setWidget(msgView);
    messageDock->close();

    QDockWidget *DOIPDock = new QDockWidget("DOIP客户端", this);
    DOIPDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    DOIPDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QWidget* lTitleBar = DOIPDock->titleBarWidget();
    QLabel *emtryLabel = new QLabel();
    DOIPDock->setTitleBarWidget(emtryLabel);
    delete lTitleBar;
    addDockWidget(Qt::RightDockWidgetArea, DOIPDock);
    DOIPDock->setWidget(doipClient.DoipWidget());

    connect(&doipClient, SIGNAL(newDoipMessage(const doipMessage_t &)),\
            msgView, SLOT(addDoipMessage(const doipMessage_t &)));

    showDoipAction = toolMenu->addAction("DOIP客户端");
    showDoipAction->setIcon(QIcon(":/icon/device.png"));
    showDoipAction->setShortcut(tr("ctrl+d"));
    connect(showDoipAction, &QAction::triggered, this, [this, DOIPDock, messageDock] {
        if (DOIPDock->isHidden()) {
            DOIPDock->show();
            if (!messageDock->isHidden()) {
                msgView->setAddDoipMessage(true);
            }
        } else {
            DOIPDock->close();
            msgView->setAddDoipMessage(false);
        }
    });

    showMsgAction = toolMenu->addAction("消息视图");
    showMsgAction->setIcon(QIcon(":/icon/communication.png"));
    showMsgAction->setShortcut(tr("ctrl+x"));
    connect(showMsgAction, &QAction::triggered, this, [this, messageDock] {
        if (messageDock->isHidden()) {
            messageDock->show();
            msgView->setAddDoipMessage(true);
        } else {
            messageDock->close();
            msgView->setAddDoipMessage(false);
        }
    });

    MD5SUMAction = toolMenu->addAction("MD5计算");
    MD5SUMAction->setIcon(QIcon(":/icon/MD5.png"));
    MD5widget = new QWidget();
    MD5widget->setAttribute(Qt::WA_QuitOnClose, false);
    MD5widget->setWindowTitle("文件MD5计算");
    MD5widget->setWindowIcon(QIcon(":/icon/MD5.png"));
    MD5widget->setStyleSheet(".QWidget{background-color: #FFFFFF;}");
    QVBoxLayout *MD5widgetLayout = new QVBoxLayout();
    MD5widget->setLayout(MD5widgetLayout);
    QLineEdit *MD5ValLineedit = new QLineEdit();
    MD5ValLineedit->setMinimumWidth(220);
    MD5widgetLayout->addWidget(MD5ValLineedit);
    FileSelect *fileSelectWidget = new FileSelect();
    MD5widgetLayout->addWidget(fileSelectWidget);
    MD5widget->resize(600, 150);
    connect(fileSelectWidget, &FileSelect::fileChange, this, [MD5ValLineedit](QString fileName){
        QFile theFile(fileName);

        if (theFile.size() > 100 * 1024 * 1024) {
            switch (QMessageBox::information(0, tr("MD5计算"), tr("文件较大计算MD5耗时较长是否继续?"), tr("是"), tr("否"), 0, 1 )) {
              case 0: break;
              case 1: default: return ; break;
            }
        }
        theFile.open(QIODevice::ReadOnly);
        QByteArray ba = QCryptographicHash::hash(theFile.readAll(), QCryptographicHash::Md5);
        theFile.close();
        qDebug() << ba.toHex().constData();
        MD5ValLineedit->setText(ba.toHex().constData());
    });
    connect(MD5SUMAction, &QAction::triggered, this, [this] {
        if (MD5widget->isHidden()) {
            MD5widget->show();
        } else {
            MD5widget->hide();
        }
    });

    charHexTransform = toolMenu->addAction("ASCII十六进制互转");
    charHexTransform->setIcon(QIcon(":/icon/switch.png"));
    toolCharHexTransform->setWindowIcon(QIcon(":/icon/switch.png"));
    toolCharHexTransform->setAttribute(Qt::WA_QuitOnClose, false);
    toolCharHexTransform->resize(800, 480);
    connect(charHexTransform, &QAction::triggered, this, [this] {
        if (toolCharHexTransform->isHidden()) {
            toolCharHexTransform->show();
        } else {
            toolCharHexTransform->hide();
        }
    });

    toolRealTimeLogAction = toolMenu->addAction("实时日志");
    toolRealTimeLogAction->setIcon(QIcon(":/icon/log.png"));
    toolRealTimeLog->setWindowIcon(QIcon(":/icon/log.png"));
    toolRealTimeLog->setAttribute(Qt::WA_QuitOnClose, false);
    toolRealTimeLog->resize(800, 480);
    connect(toolRealTimeLogAction, &QAction::triggered, this, [this] {
        if (toolRealTimeLog->isHidden()) {
            toolRealTimeLog->show();
        } else {
            toolRealTimeLog->hide();
        }
    });

    duplicateAppAction = toolMenu->addAction("应用多开");
    duplicateAppAction->setIcon(QIcon(":/icon/duplicate.png"));
    connect(duplicateAppAction, &QAction::triggered, this, [this] {
        switch (QMessageBox::information(0, tr("应用多开"), tr("再打开一个新窗口?"), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default: return ; break;
        }

        QProcess::startDetached(appFilepath, QStringList());
    });

    useAuthCode = new UseAuthCode();
    useAuthCodeAction = toolMenu->addAction("生成授权码");
    useAuthCodeAction->setIcon(QIcon(":/icon/author.png"));
    connect(useAuthCodeAction, &QAction::triggered, this, [this] {
        if (this->loginWdiget->loginUserType() != Login::AdminUser) {
            QMessageBox::warning(nullptr, tr("权限不足"), tr("用户权限不足"));
            return ;
        }
        useAuthCode->show();
    });

    aboutMenu->addAction("Version: " + QString(SOFT_VERSION))->setIcon(QIcon(":/icon/version.png"));
    aboutMenu->addAction("Compile time: " + QString(__DATE__) + " " + QString(__TIME__))->setIcon(QIcon(":/icon/date.png"));
    aboutMenu->addAction("Author: yeyong")->setIcon(QIcon(":/icon/author.png"));

    QAction *userManualAction = aboutMenu->addAction("使用手册");
    userManualAction->setIcon(QIcon(":/icon/manual.png"));
    userManualAction->setShortcut(tr("ctrl+m"));
    connect(userManualAction, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile("./serviceManual/index.html"));
    });

    upgradeWidget = new upgrade();
    QAction *updateAction = aboutMenu->addAction("软件更新检查");
    updateAction->setIcon(QIcon(":/icon/upgrade.png"));
    connect(updateAction, &QAction::triggered, this, [this] {
        if (upgradeWidget->isHidden()) {
            upgradeWidget->show();
            upgradeWidget->infoUpdate();
        } else {
            upgradeWidget->hide();
        }
    });

    QAction *udsSetConfig = fileMenu->addAction("UDS服务集合配置目录");
    udsSetConfig->setIcon(QIcon(":/icon/dir.png"));
    udsSetConfig->setShortcut(tr("ctrl+u"));
    connect(udsSetConfig, &QAction::triggered, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(uds.serviceSetProjectDir));
    });

    QAction *udsDidDescConfig = fileMenu->addAction("DID描述文件目录");
    udsDidDescConfig->setIcon(QIcon(":/icon/dir.png"));
    QDir diddir("./dictionaryEncoding/DIDDictionary");
    if (!diddir.exists()) {
        diddir.mkdir("./dictionaryEncoding/DIDDictionary");
    }
    connect(udsDidDescConfig, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile("./dictionaryEncoding/DIDDictionary"));
    });

    QAction *udsDTCDescConfig = fileMenu->addAction("DTC描述文件目录");
    udsDTCDescConfig->setIcon(QIcon(":/icon/dir.png"));
    QDir dtcdir("./dictionaryEncoding/DTCDictionary");
    if (!dtcdir.exists()) {
        dtcdir.mkdir("./dictionaryEncoding/DTCDictionary");
    }
    connect(udsDTCDescConfig, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile("./dictionaryEncoding/DTCDictionary"));
    });

    QAction *securityAccessSourceFile = fileMenu->addAction("安全会话算法示例源文件");
    securityAccessSourceFile->setIcon(QIcon(":/icon/dir.png"));
    QDir sadir("./securityAccessSourceFile/");
    if (!sadir.exists()) {
        sadir.mkdir("./securityAccessSourceFile/");
    }
    connect(securityAccessSourceFile, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile("./securityAccessSourceFile"));
    });

    QAction *securityAccessDll = fileMenu->addAction("安全会话算法dll目录");
    securityAccessDll->setIcon(QIcon(":/icon/dir.png"));
    QDir dlldir(uds.securityDllDir);
    if (!dlldir.exists()) {
        dlldir.mkdir(uds.securityDllDir);
    }
    connect(securityAccessDll, &QAction::triggered, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(uds.securityDllDir));
    });

    QAction *adbDiagBin = fileMenu->addAction("ADB诊断BIN文件目录");
    adbDiagBin->setIcon(QIcon(":/icon/dir.png"));
    QDir adbDiagdir("./bin/runtime_debug");
    if (!adbDiagdir.exists()) {
        adbDiagdir.mkdir("./bin/runtime_debug");
    }
    connect(adbDiagBin, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile("./bin/runtime_debug"));
    });

    QAction *serviceProjectListConfig = fileMenu->addAction("测试项列表文件目录");
    serviceProjectListConfig->setIcon(QIcon(":/icon/dir.png"));
    QDir serviceProjectListDir(uds.serviceProjectListConfigDir);
    if (!serviceProjectListDir.exists()) {
        serviceProjectListDir.mkdir(uds.serviceProjectListConfigDir);
    }
    connect(serviceProjectListConfig, &QAction::triggered, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(uds.serviceProjectListConfigDir));
    });

    QAction *UDSonIPSourceFile = fileMenu->addAction("源代码文件(Qt5.14.0)");
    UDSonIPSourceFile->setIcon(QIcon(":/icon/dir.png"));
    QDir sourceFile("./sourceFile");
    if (!sourceFile.exists()) {
        sourceFile.mkdir("./sourceFile");
    }
    connect(UDSonIPSourceFile, &QAction::triggered, this, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile("./sourceFile"));
    });

    QAction *udsTestResultFile = fileMenu->addAction("测试结果文件目录");
    udsTestResultFile->setIcon(QIcon(":/icon/dir.png"));
    QDir udsTestResultFileDir(uds.serviceTestResultDir);
    if (!udsTestResultFileDir.exists()) {
        udsTestResultFileDir.mkdir(uds.serviceTestResultDir);
    }
    connect(udsTestResultFile, &QAction::triggered, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(uds.serviceTestResultDir));
    });

    QAction *transferFileActive = fileMenu->addAction("传输文件目录");
    transferFileActive->setIcon(QIcon(":/icon/dir.png"));
    QDir transferFileDir(uds.servicetransferFileDir);
    if (!transferFileDir.exists()) {
        transferFileDir.mkdir(uds.servicetransferFileDir);
    }
    connect(transferFileActive, &QAction::triggered, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(uds.servicetransferFileDir));
    });

    connect(messageDock, &QDockWidget::dockLocationChanged,\
            this, [messageDock](Qt::DockWidgetArea area) {

        if (Qt::NoDockWidgetArea == area) {
            messageDock->resize(QSize(1000, 600));
        }
    });
    connect(&doipClient, SIGNAL(doipServerChange(QMap<QString, DoipClientConnect *> &)),\
            &uds, SLOT(doipServerListChangeSlot(QMap<QString, DoipClientConnect *> &)));
    connect(&doipClient, SIGNAL(diagnosisFinishSignal(DoipClientConnect *, DoipClientConnect::sendDiagFinishState, QByteArray &)),\
            &uds, SLOT(diagnosisResponseSlot(DoipClientConnect *, DoipClientConnect::sendDiagFinishState, QByteArray &)));

    softUpdate = new SoftUpdate();

    /* 设置主窗口位置和大小 */
    QSettings settings(QDir::currentPath() + "/backcache/" + "attr.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("utf-8"));
    int high = settings.value("MainWindow/height").toInt();
    int width = settings.value("MainWindow/width").toInt();
    int posx = settings.value("MainWindow/posx").toInt();
    int posy = settings.value("MainWindow/posy").toInt();
    if (high > 0 && width > 0) {
        this->resize(width, high);
    }
    if (posx > 0 && posx > 0) {
        posx = posx > rect.size().width() / 2 ? 50 : posx;
        posy = posy > rect.size().height() / 2 ? 50 : posy;
        this->move(posx, posy);
    }
    toolBarInit();
}

MainWindow::~MainWindow()
{

}

void MainWindow::toolBarInit()
{
    QString stly = \
    "QToolBar {\
    border: 0px solid gray;\
    background: white;\
    }\
    QToolBar QToolButton{\
    background:transparent;\
    border-radius:1px;\
    }\
    QToolBar QToolButton:hover{\
    background:rgb(205,205,205);\
    }\
    QToolBar QToolButton:pressed{\
    background:rgb(205,205,205);\
    }\
    QToolBar QToolButton:checked{\
    background:rgb(205,205,205);\
    }\
    QToolBar QToolButton:unchecked{\
    background:rgb(239,239,239);\
    }";
    QToolBar * toolBar = new QToolBar(this);
    toolBar->setMaximumHeight(22);
    toolBar->setStyleSheet(stly);
    this->addToolBar(toolBar);  // 将工具栏添加
    this->addToolBar(Qt::TopToolBarArea, toolBar);
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    //toolBar->setMovable(false);
    toolBar->setFloatable(false);
    toolBar->addAction(MD5SUMAction);
    toolBar->addSeparator();
    toolBar->addAction(showDoipAction);
    toolBar->addSeparator();
    toolBar->addAction(showMsgAction);
    toolBar->addSeparator();
    toolBar->addAction(charHexTransform);
    toolBar->addSeparator();
    toolBar->addAction(toolRealTimeLogAction);
    toolBar->addSeparator();
    toolBar->addAction(duplicateAppAction);

    QToolButton *logoutBtn = new QToolButton(this);
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBar->addWidget(spacer);
    QLabel *runTimeLabel = new QLabel("<font color = #03A89E><b>19时12分0秒</b></font>");
    toolBar->addWidget(runTimeLabel);
    toolBar->addWidget(logoutBtn);
    QAction *logoutactive = new QAction("退出登录");
    logoutactive->setIcon(QIcon(":/icon/logout.png"));
    logoutBtn->setDefaultAction(logoutactive);
    connect(logoutactive, &QAction::triggered, this, [this] {
        switch (QMessageBox::information(this, tr("退出登录"), tr("是否退出登录?"), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default:return;break;
        }

        if (this->loginWdiget) {
            this->loginWdiget->logout();
            this->loginWdiget->show();
            this->hide();
        }
    });

    runTtimer = new QTimer();
    runTtimer->setInterval(1000);
    runTtimer->start();
    connect(runTtimer, &QTimer::timeout, this, [this, runTimeLabel]{
        QString timeStr = "<font color = #03A89E><b>运行时长：%1时%2分%3秒</b></font>";
        qint64 runtime = startTime.secsTo(QDateTime::currentDateTime());
        timeStr = timeStr.arg(runtime / 3600).arg((runtime % 3600) / 60).arg((runtime % 3600) % 60);
        runTimeLabel->setText(QString("%1").arg(timeStr));
    });

}

void MainWindow::setLoginWidget(Login *loginWdiget)
{
    this->loginWdiget = loginWdiget;
}

void MainWindow::setAppFilepath(QString filepath)
{
    this->appFilepath = filepath;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    uds.serviceSetBackupCache();
    uds.operationCache();
    doipClient.operationCache();
    /* 保存主窗口大小和位置 */
    QSettings settings(QDir::currentPath() + "/backcache/" + "attr.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("utf-8"));
    settings.setValue("MainWindow/height", this->size().height());
    settings.setValue("MainWindow/width", this->size().width());
    settings.setValue("MainWindow/posx", this->pos().x());
    settings.setValue("MainWindow/posy", this->pos().y());

    switch (QMessageBox::information(this, tr("退出程序"), tr("确认退出?"), tr("是"), tr("否"), 0, 1 )) {
        case 0: event->accept(); break;
        case 1: default: event->ignore(); break;
    }
}

void MainWindow::hideEvent(QHideEvent *event)
{
    useAuthCode->hide();
    toolRealTimeLog->hide();
    toolCharHexTransform->hide();
    MD5widget->hide();
    upgradeWidget->hide();
}
