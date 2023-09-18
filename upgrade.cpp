#include "upgrade.h"
#include "ui_upgrade.h"
#include <QProcess>

QString webServerAddress;
quint16 webServerPort;

quint32 versionNumber(QString ver)
{
    return ver.remove('.').toUInt();
}

upgrade::upgrade(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::upgrade)
{
    ui->setupUi(this);

    ui->oldVerlabel->setText(SOFT_VERSION);
    ui->newVerLabel->setText(SOFT_VERSION);
    this->setStyleSheet(" QPushButton {\
                            background-color: #FFFFFF;\
                            color: #000000;\
                            padding:3px;\
                            border: 1px solid #1E90FF;\
                            border-radius: 3px;\
                            text-align: center;\
                        }\
                        QPushButton:hover {\
                            background-color: #33A1C9;\
                            border: 0px solid #1E90FF;\
                        }\
                        QPushButton:pressed {\
                            background-color: #1E90FF;\
                            border: 1px solid #33A1C9;\
                        }"\
                        "upgrade {background-color: #FFFFFF;}");
    ui->pushButton->setIcon(QIcon(":/icon/upgrade.png"));
    QTimer *autorefTimer = new QTimer();
    autorefTimer->setInterval(5000);
    autorefTimer->start();
    connect(autorefTimer, &QTimer::timeout, this, [this]{
        infoUpdate();
        if (isconnect) {
            ui->label_3->hide();
            ui->label_4->hide();
            ui->webPortLineEdit->hide();
            ui->webAddressLineEdit->hide();
        }
        else {
            ui->label_3->show();
            ui->label_4->show();
            ui->webPortLineEdit->show();
            ui->webAddressLineEdit->show();
        }
    });
    QSettings operationSettings("login.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
    ui->webPortLineEdit->setText(operationSettings.value("login/webServerPort").toString());
    ui->webAddressLineEdit->setText(operationSettings.value("login/webServerAddress").toString());
    if (ui->webAddressLineEdit->text().size() == 0) {
        ui->webAddressLineEdit->setText("10.10.30.195");
    }
    if (ui->webPortLineEdit->text().size() == 0) {
        ui->webPortLineEdit->setText("45621");
    }

    keepliveTimer = new QTimer();
    keepliveTimer->setInterval(10000);
    keepliveTimer->start();

    manger = new QNetworkAccessManager(this);

    connect(keepliveTimer, &QTimer::timeout, this, [this](){
        QJsonObject info = QJsonObject {
            {"MessageType", "keepAlive"},
            {"Version", SOFT_VERSION},
            {"HostInfo", QHostInfo::localHostName()},
            {"Login", Login::loginUserTypeStr()},
            {"UUID", Login::getUUID()}
        };
        QString softInfo = QString(QJsonDocument(info).toJson(QJsonDocument::Compact));;
        QNetworkRequest request;
        request.setUrl(QUrl(QString("http://%1:%2/upgrade").arg(upgrade::getWebServerAddress()).arg(upgrade::getWebServerPort())));
        //设置需要设置响应报头
        request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
        //data数据的处理流程
        QNetworkReply *reply = manger->post(request, QByteArray(softInfo.toStdString().data(), softInfo.size()));
        //设置http响应的处理动作
        connect(reply, &QNetworkReply::readyRead, this, [this, reply](){
            QByteArray replydata = reply->readAll();
            QJsonParseError err;

            isconnect = true;
            qDebug() << replydata;
            QJsonObject infoObj = QJsonDocument::fromJson(replydata, &err).object();
            if (err.error)
                return ;

            if (infoObj.contains("Command") && infoObj["Command"].toString() == "upgrade") {
                upgradeCommand(replydata);
            }
            else if (infoObj.contains("Command") && infoObj["Command"].toString() == "keepalive"){
                if (Login::loginUserType() == Login::NotLog) {
                    if (infoObj.contains("Authcode")) {
                        qDebug() << "写入授权码";
                        QSettings operationSettings("login.ini", QSettings::IniFormat);
                        operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
                        operationSettings.setValue("login/authcode", infoObj["Authcode"].toString());
                    }
                }
            }
        });
    });
}

upgrade::~upgrade()
{
    delete ui;
}

void upgrade::infoUpdate()
{
    ui->newVerLabel->setText(sg_installVersion);
    if (!(sg_downstatus == DownloadSuccess)) {
        ui->pushButton->setEnabled(false);
        ui->pushButton->hide();
        ui->newVerLabel->hide();
        ui->label->hide();
    }
    else {
        ui->pushButton->setEnabled(true);
        ui->pushButton->show();
        ui->newVerLabel->show();
        ui->label->show();
    }
}

QString upgrade::getWebServerAddress()
{
    return webServerAddress;
}

quint16 upgrade::getWebServerPort()
{
    return webServerPort;
}

bool upgrade::upgradeCommand(QByteArray replydata)
{
    QJsonParseError err;

    qDebug() << replydata;
    QJsonObject infoObj = QJsonDocument::fromJson(replydata, &err).object();
    if (err.error)
        return false;
    /* 检测是否有更新 */
    QString version = infoObj["Version"].toString();
    QString url = infoObj["Url"].toString();
    QString md5 = infoObj["MD5"].toString();
    qDebug() << "HTTP Server version:" << version;
    qDebug() << "HTTP Server url:" << url;
    if (version.size() > 0 && versionNumber(version) > versionNumber(SOFT_VERSION) && sg_downstatus == NotDownloaded) {
        sg_installVersion = version;
        /* 开始更新软件版本 */
        QFile file(sg_installFilePath);
        if (file.exists()) {
            QFile file(sg_installFilePath);
            file.open(QIODevice::ReadOnly);
            QByteArray ba = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5);
            file.close();
            if (QString(ba.toHex().constData()) == md5) {
                sg_downstatus = DownloadSuccess;
                qDebug() << "文件已经存在不用下载";
                return true;
            }
        }

        /* 开始下载文件 */
        qDebug() <<  "开始下载升级安装包:" << url;
        // 构造HTTP GET请求
        QUrl downurl(url);
        QNetworkRequest request(url);

        // 发送HTTP GET请求
        QNetworkReply *downreply = this->manger->get(request);
        QObject::connect(downreply, &QNetworkReply::finished, [this, downreply, md5] {
            if (downreply->error() == QNetworkReply::NoError) {
                // 保存下载的文件数据
                QFile file(sg_installFilePath);
                file.open(QIODevice::WriteOnly);
                QByteArray downdata = downreply->readAll();
                file.write(downdata);
                file.close();
                QByteArray ba = QCryptographicHash::hash(downdata, QCryptographicHash::Md5);
                if (QString(ba.toHex().constData()) == md5) {
                    qDebug() << "File downloaded successfully";
                    sg_downstatus = DownloadSuccess;
                }
            } else {
                qDebug() << "Error downloading file:" << downreply->errorString();
                sg_downstatus = DownloadFailed;
            }
        });
    }

    return true;
}

void upgrade::on_pushButton_clicked()
{
    QProcess::startDetached(sg_installFilePath, QStringList());
}

void upgrade::on_webPortLineEdit_textChanged(const QString &arg1)
{
    qDebug() << "textChanged( webServerPort" << arg1;
    webServerPort = arg1.toUInt();
}

void upgrade::on_webAddressLineEdit_textChanged(const QString &arg1)
{
    qDebug() << "textChanged( webServerAddress" << arg1;
    webServerAddress = arg1;
}

void upgrade::on_webAddressLineEdit_textEdited(const QString &arg1)
{
    qDebug() << "textEdited webServerAddress" << arg1;
    webServerAddress = arg1;
    QSettings operationSettings("login.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
    operationSettings.setValue("login/webServerAddress", arg1);
}

void upgrade::on_webPortLineEdit_textEdited(const QString &arg1)
{
    qDebug() << "textEdited webServerPort" << arg1;
    webServerPort = arg1.toUInt();
    QSettings operationSettings("login.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
    operationSettings.setValue("login/webServerPort", arg1);
}
