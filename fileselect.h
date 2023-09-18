#ifndef FILESELECT_H
#define FILESELECT_H

#include <QWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

namespace Ui {
class FileSelect;
}

class FileSelect : public QWidget
{
    Q_OBJECT

public:
    explicit FileSelect(QWidget *parent = nullptr);
    ~FileSelect();
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    void paintEvent(QPaintEvent *event);
private slots:
    void on_fileSelectBtn_clicked();

signals:
    void fileChange(QString filename);

private:
    Ui::FileSelect *ui;
    QString fileTransferPath;
    QString dragTriggerStyle = "QPushButton {\
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
                                }";
    QString dragReleaseStyle = "QPushButton {\
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
                                }";
};

#endif // FILESELECT_H
