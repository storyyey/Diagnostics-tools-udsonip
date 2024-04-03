#include "useauthcode.h"
#include "ui_useauthcode.h"
#include "QtAES/qaesencryption.h"
#include "softupdate.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDebug>
#include <QDateTime>

UseAuthCode::UseAuthCode(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UseAuthCode)
{
    ui->setupUi(this);
    // ui->lineEdit->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{32}$"), this));
    ui->comboBox_2->setEditable(true);
    ui->comboBox_2->setCurrentText(SOFT_VERSION);
    this->setStyleSheet(" QPushButton {\
                            background-color: #FFFFFF;\
                            color: #000000;\
                            padding:3px;\
                            border: 0px solid #FFFFFF;\
                            border-radius: 3px;\
                            text-align: center;\
                        }\
                        QPushButton:hover {\
                            background-color: #33A1C9;\
                        }\
                        QPushButton:pressed {\
                            background-color: #1E90FF;\
                        }"\
                        "UseAuthCode {background-color: #FFFFFF;}");
    ui->pushButton->setIcon(QIcon(":/icon/encrypt.png"));
    ui->pushButton_2->setIcon(QIcon(":/icon/copy.png"));
    ui->authCodeFileLabel->hide();
    ui->authCodeFileLabel->setOpenExternalLinks(true);
}

UseAuthCode::~UseAuthCode()
{
    delete ui;
}

bool UseAuthCode::saveLicenseKey(QString filename, QString lickey)
{
    QFile file(filename);

    if (file.exists()) {
        file.remove();
    }

    file.open(QIODevice::WriteOnly);
    file.write(lickey.toStdString().c_str(), lickey.size());
    file.close();

    return true;
}

void UseAuthCode::on_pushButton_clicked()
{
    QAESEncryption encryption(QAESEncryption::AES_128, QAESEncryption::ECB, QAESEncryption::ZERO);
    QByteArray hashKey = QCryptographicHash::hash(QString(ui->lineEdit->text() + "storyyey@github").toUtf8(), QCryptographicHash::Md5);

    QJsonObject authinfo = QJsonObject {
        {"ST", QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss")},
        {"ET", QDateTime::currentDateTime().addDays(ui->comboBox->currentText().toUInt()).toString("yyyy/MM/dd hh:mm:ss")},
        {"VR", ui->comboBox_2->currentText()}
    };
    qDebug() << "authinfo:" << QString(QJsonDocument(authinfo).toJson(QJsonDocument::Compact)).toUtf8().data();
    qDebug() << "hashKey" << hashKey;

    QByteArray encodedText = encryption.encode(QString(QJsonDocument(authinfo).toJson(QJsonDocument::Compact)).toUtf8(), hashKey);
    QString enauthinfo = QString::fromLatin1(encodedText.toBase64());
    qDebug() << "en authinfo:" << enauthinfo;
    ui->plainTextEdit->setPlainText(enauthinfo);

    QDir dir(licenseKeyDir);
    if (!dir.exists()) {
        dir.mkdir(licenseKeyDir);
    }
    if (saveLicenseKey(licenseKeyDir + "/" + ui->lineEdit->text() + ".lic", enauthinfo)) {
        ui->authCodeFileLabel->show();
        ui->authCodeFileLabel->setText(QString("<a style='color: rgb(30, 144, 255);' href=\"file:///%1\">%2</a>").arg(licenseKeyDir).arg(ui->lineEdit->text() + ".lic"));
    }
}

void UseAuthCode::on_pushButton_2_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->plainTextEdit->toPlainText());
}
