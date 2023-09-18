#ifndef USEAUTHCODE_H
#define USEAUTHCODE_H

#include <QWidget>
#include <QDir>

namespace Ui {
class UseAuthCode;
}

class UseAuthCode : public QWidget
{
    Q_OBJECT

public:
    explicit UseAuthCode(QWidget *parent = nullptr);
    ~UseAuthCode();
    bool saveLicenseKey(QString filename, QString lickey);
private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::UseAuthCode *ui;
    QString licenseKeyDir = QDir::currentPath() + "/licenseKey/";
};

#endif // USEAUTHCODE_H
