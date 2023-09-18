#include "fileselect.h"
#include "ui_fileselect.h"

FileSelect::FileSelect(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileSelect)
{
    ui->setupUi(this);
    this->setAcceptDrops(true);
    this->setToolTip("拖拽文件到此处自动识别");
    this->setStyleSheet(dragReleaseStyle);
    this->setAttribute(Qt::WA_StyledBackground);
    ui->filenameLabel->setOpenExternalLinks(true);
}

FileSelect::~FileSelect()
{
    delete ui;
}

void FileSelect::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
    this->setStyleSheet(dragTriggerStyle);
    for(int i = 0; i < event->mimeData()->urls().length(); i++)
    {
        QString fileName = event->mimeData()->urls().at(i).toLocalFile();//获取拖拽文件路径名
        ui->filenameLabel->setText(fileName);
    }
}

void FileSelect::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    this->setStyleSheet(dragReleaseStyle);
    ui->filenameLabel->setText("拖拽文件到这里选择文件");
}

#include <QFileInfo>

void FileSelect::dropEvent(QDropEvent *event)
{
    for(int i = 0; i < event->mimeData()->urls().length(); i++)
    {
        QString fileName = event->mimeData()->urls().at(i).toLocalFile();//获取拖拽文件路径名
        ui->filenameLabel->setText(QString("<a style='color: rgb(30, 144, 255);' href=\"file:///%1\">%2</a>").\
                                   arg(QFileInfo(fileName).path()).arg(fileName));
        emit fileChange(fileName);
    }
    this->setStyleSheet(dragReleaseStyle);
}
#include <QPaintEvent>
#include <QPainter>
void FileSelect::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(rect(),QPixmap(":/icon/file_select_bk.png"));
}
#include <QFileDialog>
void FileSelect::on_fileSelectBtn_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择文件", \
                         fileTransferPath.size() == 0 ? "C:\\Users\\Administrator\\Desktop" : fileTransferPath);
    if (fileName.size() == 0) { return ; }
    QFileInfo fileInfo(fileName);

    fileTransferPath = fileInfo.path();
    ui->filenameLabel->setText(QString("<a style='color: rgb(30, 144, 255);' href=\"file:///%1\">%2</a>").\
                               arg(fileInfo.path()).arg(fileName));
    emit fileChange(fileName);
}
