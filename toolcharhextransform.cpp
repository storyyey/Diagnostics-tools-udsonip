#include "toolcharhextransform.h"
#include "ui_toolcharhextransform.h"

ToolCharHexTransform::ToolCharHexTransform(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ToolCharHexTransform)
{
    ui->setupUi(this);
    ui->hexPlainTextEdit->setPlaceholderText("请输入十六进制字符");
    ui->charPlainTextEdit->setPlaceholderText("请输入ASCII字符");
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
                        "ToolCharHexTransform {background-color: #FFFFFF;}");

}

ToolCharHexTransform::~ToolCharHexTransform()
{
    delete ui;
}

void ToolCharHexTransform::on_hexToCharBtn_clicked()
{
    QString hexstr = ui->hexPlainTextEdit->toPlainText();
    if (hexstr.size() == 0) return ;
    QByteArray msg = QByteArray::fromHex(hexstr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
    ui->charPlainTextEdit->setPlainText(QString(msg));
    ui->label_2->setText(QString("%1").arg(msg.size()));
}

void ToolCharHexTransform::on_charToHexBtn_clicked()
{
    QString charstr = ui->charPlainTextEdit->toPlainText();
    if (charstr.size() == 0) return ;
    QByteArray msg;
    msg.append(charstr);
    if (ui->hexSpaceCheckBox->checkState() == Qt::Checked) {
        ui->hexPlainTextEdit->setPlainText(msg.toHex(' '));
    } else {
         ui->hexPlainTextEdit->setPlainText(msg.toHex());
    }
    ui->label_4->setText(QString("%1").arg(charstr.size()));
}

void ToolCharHexTransform::on_clearPushButton_clicked()
{
    ui->hexPlainTextEdit->clear();
}

void ToolCharHexTransform::on_hexPlainTextEdit_textChanged()
{
    QString hexstr = ui->hexPlainTextEdit->toPlainText();
    if (hexstr.size() == 0) return ;
    QByteArray msg = QByteArray::fromHex(hexstr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
    ui->label_2->setText(QString("%1").arg(msg.size()));
}

void ToolCharHexTransform::on_charPlainTextEdit_textChanged()
{
    QString charstr = ui->charPlainTextEdit->toPlainText();
    ui->label_4->setText(QString("%1").arg(charstr.size()));
}

void ToolCharHexTransform::on_pushButton_clicked()
{
    ui->charPlainTextEdit->clear();
}
