#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QMovie>
#include <QMainWindow>
#include <QFileInfo>
#include <QDateTime>
#include <QSettings>
#include <QTextCodec>
#include <QCryptographicHash>
#include <QJsonParseError>
#include "softupdate.h"
#include "QtAES/qaesencryption.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>

namespace Ui {
class Login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();
    void setMainWindow(QMainWindow *windown);
    void autoLogin();
    void loginCheck(bool record);
    bool loginCheck(QString username, QString password, bool record);
    enum UserType{
        NotLog = 0,
        NormalUser,
        AuthUser,
        AdminUser
    };
    static enum UserType loginUserType();
    static QString loginUserTypeStr();
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void hideEvent(QHideEvent *event);
    void logout();
    void closeEvent(QCloseEvent *event);
    bool lastLoginRecord(QString licence, QString version);
    bool lastLoginValid(QString licence, QString version);
    static QString getUUID();
private slots:
    void on_loginPushButton_clicked();
    void on_userNamelineEdit_textEdited(const QString &arg1);

    void on_passwordlineEdit_textEdited(const QString &arg1);

    void on_userNamelineEdit_textChanged(const QString &arg1);

    void on_passwordlineEdit_textChanged(const QString &arg1);

    void on_keyLineEdit_textEdited(const QString &arg1);

    void on_licencePushButton_clicked();

    void on_useLicCheckBox_stateChanged(int arg1);

private:
    Ui::Login *ui;
    QMainWindow *windown = nullptr;
    QTimer *autoLoginTimer = nullptr;
    QString licenceUseHint = "※Licence粘贴到输入框内点击Licence按钮\n或者点击Licence按钮选择Licence文件";
};

#endif // LOGIN_H
