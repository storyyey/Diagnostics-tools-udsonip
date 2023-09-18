#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QCryptographicHash>
#include <QToolBar>
#include <QToolButton>
#include "messageview.h"
#include "uds.h"
#include "doipclient.h"
#include "softupdate.h"
#include "toolcharhextransform.h"
#include "toolrealtimelog.h"
#include "useauthcode.h"
#include "login.h"
#include "upgrade.h"

void realTimeLog(QString log);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void toolBarInit();
    void setLoginWidget(Login *loginWdiget);
    void setAppFilepath(QString filepath);
    void closeEvent(QCloseEvent *event);
    void hideEvent(QHideEvent *event);
private:
    MessageView *msgView;
    DoipClient doipClient;
    UDS uds;
    SoftUpdate *softUpdate = nullptr;
    QAction *MD5SUMAction = nullptr;
    QAction *showDoipAction = nullptr;
    QAction *showMsgAction = nullptr;
    QAction *charHexTransform = nullptr;
    QAction *toolRealTimeLogAction = nullptr;
    QAction *duplicateAppAction = nullptr;
    QAction *useAuthCodeAction = nullptr;
    Login *loginWdiget = nullptr;
    QTimer *runTtimer = nullptr;
    QDateTime startTime = QDateTime::currentDateTime();
    ToolCharHexTransform *toolCharHexTransform = nullptr;
    ToolRealTimeLog *toolRealTimeLog = nullptr;
    QString appFilepath;
    UseAuthCode *useAuthCode = nullptr;
    QWidget *MD5widget = nullptr;
    upgrade *upgradeWidget = nullptr;
};
#endif // MAINWINDOW_H
