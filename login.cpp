#include "login.h"
#include "ui_login.h"
#include <QHostInfo>
#include <QProcess>
#include <QFileDialog>

static QString devuuid = "";
static Login::UserType usertype = Login::NotLog;

QString _getComputerUUID()
{
    QProcess process;

#ifdef Q_OS_WIN
//    wmic csproduct get uuid

    QStringList mList;
    mList << "csproduct" << "get" << "uuid";
    process.start("wmic", mList);

    process.waitForFinished();
    QString ret = process.readAll();
    ret = ret.trimmed();

    QStringList dataList = ret.split("\r\n");

    if(dataList.length() != 2)
    {
        return "";
    }

    return dataList[1].trimmed();
#endif

#ifdef Q_OS_LINUX
    QStringList mList;
    mList << "-s" << "system-uuid";
    process.start("dmidecode", mList);

    process.waitForFinished();
    QString ret = process.readAll();
    ret = ret.trimmed();

    return ret;
#endif

    return "";
}

Login::Login(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Login)
{
    devuuid = _getComputerUUID();
    ui->setupUi(this);
    QMovie *movie = new QMovie(":/icon/login1.gif");
    ui->logLabel->setMovie(movie);
    movie->start();
    //ui->logLabel->setFixedSize(200, 100);
    this->setStyleSheet("background-color:white;");
    //this->setFixedSize(400, 230);
    //movie->setScaledSize(QSize(280, 200));
    QString lineEditStyle = "QLineEdit {border: 1px solid #8B8378;border-radius: 2px; width: 200px; height: 20px;}\
                             QLineEdit:focus{border: 1px solid #63B8FF;border-radius: 2px;  width: 200px; height: 20px;}";
    QString pushButtonStyle = ".QPushButton:hover {background-color: #D3D3D3;border-radius: 2px; width: 50px; height: 25px;}\
                              .QPushButton:pressed {background-color: #F5F5F5;border-radius: 2px; width: 50px; height: 25px;}\
                              .QPushButton {background-color: transparent;height:20px;border-radius: 2px;border: 1px solid #8B8378; width: 50px; height: 25px;}";

    ui->userNamelineEdit->setStyleSheet("QLineEdit { background-color : transparent }");
    ui->passwordlineEdit->setStyleSheet("QLineEdit { background-color : transparent }");
    ui->passwordlineEdit->setEchoMode(QLineEdit::Password);
    ui->loginPushButton->setStyleSheet(pushButtonStyle);
    ui->passwordLabel->setStyleSheet("QLabel { background-color : transparent }");
    ui->userNameLabel->setStyleSheet("QLabel { background-color : transparent }");
    ui->licencelabel->setStyleSheet("QLabel { background-color : transparent }");
    // ui->loginPushButton->hide();
    ui->keyLineEdit->setStyleSheet("QLineEdit {border: 0px solid transparent;}");
    ui->infoLabel->setStyleSheet("QLineEdit {border: 0px solid transparent;}");
    ui->useLicCheckBox->setStyleSheet("QCheckBox { background-color : transparent }");
    ui->rememberUserCheckBox->setStyleSheet("QCheckBox { background-color : transparent }");
    ui->LicenceTextEdit->setStyleSheet("QTextEdit { background-color : transparent }");

    QFileInfo info("login.ini");
    if (info.exists()) {
        QDateTime time1 = info.lastModified();
        QDateTime time2 = QDateTime::currentDateTime();
        qDebug() << time1;
        qDebug() << time2;
        int days = time1.daysTo(time2);
        qDebug() << "time2 - time1=" << days ;
        if (days >= 7) {
            QSettings operationSettings("login.ini", QSettings::IniFormat);
            operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
            operationSettings.setValue("login/userName", "");
            operationSettings.setValue("login/password", "");
        }
    }

    QString key(devuuid);
    QByteArray hashKey = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5);
    ui->keyLineEdit->setText(devuuid);
    ui->infoLabel->setOpenExternalLinks(true);
    ui->infoLabel->setText(QString("<a style='color: blue;' href=''><u></u></a>" + QString(QString(" Version:") + SOFT_VERSION)));
    ui->loginPushButton->setIcon(QIcon(":/icon/login.png"));
    ui->loginHintLabel->setText(licenceUseHint);
    ui->licencePushButton->setStyleSheet(pushButtonStyle);
    ui->licencePushButton->setIcon(QIcon(":/icon/licence.png"));
    ui->LicenceTextEdit->hide();
    ui->licencelabel->hide();
    ui->licencePushButton->hide();

    QSettings operationSettings("login.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
    ui->useLicCheckBox->setCheckState((Qt::CheckState)operationSettings.value("login/useLicCheckBox").toInt());

    autoLoginTimer = new QTimer();
    autoLoginTimer->setInterval(20000);
    autoLoginTimer->start();
    connect(autoLoginTimer, &QTimer::timeout, this, [this]{
        /* 自动使用授权码登录 */
        ui->useLicCheckBox->setCheckState(Qt::Checked);
        autoLogin();
        if (Login::loginUserType() != Login::NotLog) {
            autoLoginTimer->stop();
        }
    });
}

Login::~Login()
{
    delete ui;
}

void Login::setMainWindow(QMainWindow *windown)
{
    this->windown = windown;
}

void Login::autoLogin()
{
    if (windown) {
        windown->show();
        this->hide();
        return ;
    }

    QSettings operationSettings("login.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));

    if (Qt::Checked == ui->useLicCheckBox->checkState()) {
        ui->LicenceTextEdit->setText(operationSettings.value("login/authcode").toString());
        loginCheck("", "", false);
    }
    else {
        if (!loginCheck(operationSettings.value("login/userName").toString(), \
                       operationSettings.value("login/password").toString(), false)) {
        }
    }
}

void Login::loginCheck(bool record)
{
    Q_UNUSED(record);
}

bool Login::loginCheck(QString username, QString password, bool record)
{
    qDebug() << "username:" << username << "password:" << password << "record:" << record;
    QString key(devuuid);
    QAESEncryption encryption(QAESEncryption::AES_128, QAESEncryption::ECB, QAESEncryption::ZERO);
    QByteArray hashKey = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5);

    if (Qt::Checked == ui->useLicCheckBox->checkState()) {
        QJsonParseError err;

        QString licenceCode = ui->LicenceTextEdit->toPlainText();
        if (!lastLoginValid(licenceCode, SOFT_VERSION)) {
            ui->loginHintLabel->setText(QString("<font color = red>系统信息错误无法使用Licence，请重新校对系统时间</font>"));
            return false;
        }

        QString deauthcode = QString::fromUtf8(encryption.decode(QByteArray::fromBase64(licenceCode.toLatin1()), hashKey));
        QJsonObject authObj = QJsonDocument::fromJson(deauthcode.toUtf8(), &err).object();
        if (err.error) {
            ui->loginHintLabel->setText(QString("<font color = red>Licence信息异常</font>"));
            return false;
        }

        qDebug() << QString(QJsonDocument(authObj).toJson()).toUtf8().data();
        if (!authObj.contains("VR") || authObj["VR"] != SOFT_VERSION) {
            qDebug() << "授权码版本信息错误：" << authObj["VR"];
            ui->loginHintLabel->setText(QString("<font color = red>Licence版本信息异常</font>"));
            return false;
        }
        if (!authObj.contains("ST") || !authObj.contains("ET")) {
            qDebug() << "授权码内容错误";
            ui->loginHintLabel->setText(QString("<font color = red>Licence无效</font>"));
            return false;
        }
        QString startTime = authObj["ST"].toString();
        QString endTime = authObj["ET"].toString();
        qDebug() << "startTime:" << startTime;
        qDebug() << "endTime" << endTime;

        qDebug() << QDateTime::fromString(startTime, "yyyy/MM/dd hh:mm:ss");
        qDebug() << QDateTime::fromString(endTime, "yyyy/MM/dd hh:mm:ss");
        QDateTime startDateTime = QDateTime::fromString(startTime, "yyyy/MM/dd hh:mm:ss");
        QDateTime endDateTime = QDateTime::fromString(endTime, "yyyy/MM/dd hh:mm:ss");
        QDateTime currDateTime = QDateTime::currentDateTime();
        qDebug() << currDateTime.secsTo(startDateTime);
        qDebug() << currDateTime.secsTo(endDateTime);
        if (startDateTime.secsTo(currDateTime) > 0 && endDateTime.secsTo(currDateTime) < 0) {
            if (windown) {
                windown->show();
                this->hide();
                usertype = AuthUser;
                ui->loginHintLabel->setText(QString("<font color = green>Licence登录成功</font>"));
                QSettings operationSettings("login.ini", QSettings::IniFormat);
                operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
                operationSettings.setValue("login/authcode", licenceCode);
                lastLoginRecord(licenceCode, SOFT_VERSION);

                return true;
            }
        }
        else {
            ui->loginHintLabel->setText(QString("<font color = red>Licence已经过期</font>"));
        }
        return false;
    }
    else {
        if (username.size() == 0 && password.size() == 0) {
            // ui->loginHintLabel->setText(QString("<font color = red>用户名或密码错误</font>"));
            return false;
        }

        QString enNormalUser = QString::fromLatin1(encryption.encode(QString("root").toUtf8(), hashKey).toBase64());
        QString enNormalPassword = QString::fromLatin1(encryption.encode(QString("").toUtf8(), hashKey).toBase64());

        QString enAdminUser = QString::fromLatin1(encryption.encode(QString("admin").toUtf8(), hashKey).toBase64());
        QString enAdminPassword = QString::fromLatin1(encryption.encode(QString("").toUtf8(), hashKey).toBase64());

        if ((QString(username).toUtf8() == enNormalUser && \
            QString(password).toUtf8() == enNormalPassword) ||\
            (QString::fromLatin1(encryption.encode(QString(username).toUtf8(), hashKey).toBase64()) == enNormalUser && \
             QString::fromLatin1(encryption.encode(QString(password).toUtf8(), hashKey).toBase64()) == enNormalPassword)) {
            if (windown) {
                windown->show();
                this->hide();
                usertype = NormalUser;
                ui->loginHintLabel->setText(QString("<font color = green>普通用户登录成功</font>"));
                if (ui->rememberUserCheckBox->checkState() == Qt::Checked && record) {
                    QSettings operationSettings("login.ini", QSettings::IniFormat);
                    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
                    operationSettings.setValue("login/userName", enNormalUser);
                    operationSettings.setValue("login/password", enNormalPassword);
                }
                return true;
            }
        }
        else if ((QString(username).toUtf8() == enAdminUser && \
                  QString(password).toUtf8() == enAdminPassword) ||\
                 (QString::fromLatin1(encryption.encode(QString(username).toUtf8(), hashKey).toBase64()) == enAdminUser && \
                 QString::fromLatin1(encryption.encode(QString(password).toUtf8(), hashKey).toBase64()) == enAdminPassword)) {
            if (windown) {
                windown->show();
                this->hide();
                usertype = AdminUser;
                ui->loginHintLabel->setText(QString("<font color = green>管理员用户登录成功</font>"));
                if (ui->rememberUserCheckBox->checkState() == Qt::Checked && record) {
                    QSettings operationSettings("login.ini", QSettings::IniFormat);
                    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
                    operationSettings.setValue("login/userName", enNormalUser);
                    operationSettings.setValue("login/password", enNormalPassword);
                }

                return true;
            }
        }
        ui->loginHintLabel->setText(QString("<font color = red>用户名或密码错误</font>"));
    }

    return false;
}

Login::UserType Login::loginUserType()
{
    return usertype;
}

QString Login::loginUserTypeStr()
{
    if (NotLog == usertype) {
        return "Not login";
    }
    else if (NormalUser == usertype) {
        return "Normal user";
    }
    else if (AuthUser == usertype) {
        return "Licence user";
    }
    else if (AdminUser == usertype) {
        return "Admin user";
    }
    else {
        return "Unknown user";
    }
}

void Login::dropEvent(QDropEvent *event)
{
    for(int i = 0; i < event->mimeData()->urls().length(); i++)
    {
        QString fileName = event->mimeData()->urls().at(i).toLocalFile();
        QFile file(fileName);

        file.open(QIODevice::ReadOnly);
        QByteArray lickey = file.readAll();
        ui->userNamelineEdit->setText(lickey);
        file.close();
    }
}

void Login::dragEnterEvent(QDragEnterEvent *event)
{
    Q_UNUSED(event);
}

void Login::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
}

void Login::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event);
}

void Login::logout()
{
    if (ui->rememberUserCheckBox->checkState() != Qt::Checked) {
        ui->userNamelineEdit->clear();
        ui->passwordlineEdit->clear();
        QSettings operationSettings("login.ini", QSettings::IniFormat);
        operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
        operationSettings.setValue("login/userName", "");
        operationSettings.setValue("login/password", "");
    }
    if (Qt::Checked == ui->useLicCheckBox->checkState()) {
        ui->loginHintLabel->setText(licenceUseHint);
    }
    else if (usertype == AdminUser) {
        ui->loginHintLabel->setText("管理员退出登录");
    }
    else if (usertype == NormalUser) {
         ui->loginHintLabel->setText("普通用户退出登录");
    }
    usertype = Login::NotLog;
}

void Login::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
}

bool Login::lastLoginRecord(QString licence, QString version)
{
    QString key(devuuid);
    QAESEncryption encryption(QAESEncryption::AES_128, QAESEncryption::ECB, QAESEncryption::ZERO);
    QByteArray hashKey = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5);

    QJsonObject logininfo = QJsonObject {
        {"Licence", licence},
        {"LastTime", QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss")},
        {"Version", version}
    };

    QByteArray enlogininfoText = encryption.encode(QString(QJsonDocument(logininfo).toJson(QJsonDocument::Compact)).toUtf8(), hashKey);
    QString logininfoStr = QString::fromLatin1(enlogininfoText.toBase64());

    QSettings operationSettings("login.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
    operationSettings.setValue("login/loginInfo", logininfoStr);

    return true;
}

bool Login::lastLoginValid(QString licence, QString version)
{
    Q_UNUSED(version);
    QJsonParseError err;
    QString key(devuuid);
    QAESEncryption encryption(QAESEncryption::AES_128, QAESEncryption::ECB, QAESEncryption::ZERO);
    QByteArray hashKey = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5);

    QSettings operationSettings("login.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
    QString logininfoStr = operationSettings.value("login/loginInfo").toString();
    if (logininfoStr.size() == 0) {
        /* 首次登录直接登录 */
        return true;
    }

    QString deloginInfo = QString::fromUtf8(encryption.decode(QByteArray::fromBase64(logininfoStr.toLatin1()), hashKey));
    QJsonObject deloginInfoObj = QJsonDocument::fromJson(deloginInfo.toUtf8(), &err).object();
    if (err.error) {
        /* 被修改过无法解密数据 */
        return false;
    }

    if (!deloginInfoObj.contains("Licence") ||\
        !deloginInfoObj.contains("LastTime") ||\
        !deloginInfoObj.contains("Version")) {
        /* 登录信息存在异常，禁止登录 */
        return false;
    }

    QDateTime lastDateTime = QDateTime::fromString(deloginInfoObj["LastTime"].toString(), "yyyy/MM/dd hh:mm:ss");
    QDateTime currDateTime = QDateTime::currentDateTime();
    if (lastDateTime.secsTo(currDateTime) < 0) {
        /* 登录时间存在异常，禁止登录 */
        return false;
    }

    if (licence != deloginInfoObj["Licence"].toString()) {
        /* 更换授权码直接登录 */
        return true;
    }

    return true;
}

QString Login::getUUID()
{
    return devuuid;
}

void Login::on_loginPushButton_clicked()
{
    autoLogin();
    if (!loginCheck(ui->userNamelineEdit->text(), ui->passwordlineEdit->text(), true)) {
        QSettings operationSettings("login.ini", QSettings::IniFormat);
        operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
        loginCheck(operationSettings.value("login/authcode").toString(), \
                           operationSettings.value("login/authcode").toString(), true);
    }
}

void Login::on_userNamelineEdit_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1);
}

void Login::on_passwordlineEdit_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1);
    // loginCheck(ui->userNamelineEdit->text(), ui->passwordlineEdit->text(), true);
}

void Login::on_userNamelineEdit_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
}

void Login::on_passwordlineEdit_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
}

void Login::on_keyLineEdit_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1);
}

void Login::on_licencePushButton_clicked()
{
    if (ui->LicenceTextEdit->toPlainText().size() > 0) {
        ui->loginPushButton->click();
        return ;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "使用licence", "C:\\Users\\Administrator\\Desktop", tr("lic (*.lic)"));
    if (!fileName.size()) {
        return ;
    }
    QFile file(fileName);

    file.open(QIODevice::ReadOnly);
    QByteArray lickey = file.readAll();
    ui->LicenceTextEdit->setText(lickey);
    file.close();
    ui->useLicCheckBox->setCheckState(Qt::Checked);
    ui->loginPushButton->click();
}

void Login::on_useLicCheckBox_stateChanged(int arg1)
{
    QSettings operationSettings("login.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
    operationSettings.setValue("login/useLicCheckBox", ui->useLicCheckBox->checkState());
    if (Qt::Checked == arg1) {
        ui->loginHintLabel->setText(licenceUseHint);
        ui->LicenceTextEdit->setText(operationSettings.value("login/authcode").toString());
        ui->LicenceTextEdit->show();
        ui->licencelabel->show();
        ui->passwordlineEdit->hide();
        ui->userNamelineEdit->hide();
        ui->passwordLabel->hide();
        ui->userNameLabel->hide();
        ui->licencePushButton->show();
        ui->loginPushButton->hide();
    } else {
        ui->loginHintLabel->clear();
        ui->LicenceTextEdit->hide();
        ui->licencelabel->hide();
        ui->passwordlineEdit->show();
        ui->userNamelineEdit->show();
        ui->passwordLabel->show();
        ui->userNameLabel->show();
        ui->licencePushButton->hide();
        ui->loginPushButton->show();
    }
}
