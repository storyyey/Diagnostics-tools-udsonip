#include "toolrealtimelog.h"
#include "ui_toolrealtimelog.h"

ToolRealTimeLog::ToolRealTimeLog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ToolRealTimeLog)
{
    ui->setupUi(this);
    ui->logPlainTextEdit->setMaximumBlockCount(10000);
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
                        "ToolRealTimeLog {background-color: #FFFFFF;}");
    ui->saveLogBtn->setIcon(QIcon(":/icon/save.png"));
}

ToolRealTimeLog::~ToolRealTimeLog()
{
    delete ui;
}

void ToolRealTimeLog::realtimeLog(QString log)
{
    if (!this->isHidden()) {
        ui->logPlainTextEdit->appendPlainText(log);
    }
}
