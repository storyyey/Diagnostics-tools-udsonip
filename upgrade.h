#ifndef UPGRADE_H
#define UPGRADE_H

#include <QWidget>
#include <QThread>
#include <QUdpSocket>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QCheckBox>
#include <QByteArray>
#include <QDateTime>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVariant>
#include <QDir>
#include <QTimer>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QHostInfo>

#include "softupdate.h"
#include "login.h"

namespace Ui {
class upgrade;
}

class upgrade : public QWidget
{
    Q_OBJECT

public:
    explicit upgrade(QWidget *parent = nullptr);
    ~upgrade();
    void infoUpdate();
    static QString getWebServerAddress();
    static quint16 getWebServerPort();
    enum installFileStatus {
        NotDownloaded,
        BeDownloading,
        DownloadFailed,
        DownloadSuccess
    };
    static QString getInstallFilePath();
    static QString getInstallFileVersion();
    static bool isUpgrade();
    static bool isConnect();
    bool upgradeCommand(QByteArray replydata);
private slots:
    void on_pushButton_clicked();

    void on_webPortLineEdit_textChanged(const QString &arg1);

    void on_webAddressLineEdit_textChanged(const QString &arg1);

    void on_webAddressLineEdit_textEdited(const QString &arg1);

    void on_webPortLineEdit_textEdited(const QString &arg1);

private:
    Ui::upgrade *ui;
    QTimer *keepliveTimer = nullptr;
    QNetworkAccessManager *manger = nullptr;
    QString sg_installFilePath = QDir::currentPath() + "/DiagToolInstalll.exe";
    enum installFileStatus sg_downstatus = NotDownloaded;
    QString sg_installVersion = SOFT_VERSION;
    bool isconnect = false;
};

#endif // UPGRADE_H
