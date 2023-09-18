#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include <QWidget>
#include <QStandardItemModel>
#include <QStringList>
#include <QTableView>
#include <QHeaderView>
#include <QHostAddress>
#include <QMessageBox>

enum doipMessageState {
    doipMessageNormal,
    doipMessageWarning,
    doipMessageError,
};

typedef struct doipMessage_s {
    QString direction;
    QString time;
    QHostAddress address;
    quint16 port;
    QString msgType;
    QByteArray msg;
    doipMessageState state;
} doipMessage_t;

class MessageView : public QWidget
{
    Q_OBJECT
public:
    explicit MessageView(QWidget *parent = nullptr);
    void setAddDoipMessage(bool add);
signals:
private slots:
   void addDoipMessage(const doipMessage_t &val);
private:
    bool isAddDoipMessage = false;
    QStringList sensorTypeList;
    QStringList UDSitemTitle;
    QStandardItemModel *UDStabModel;
    QTableView *UDStabView;

    QStringList DOIPitemTitle;
    QStandardItemModel *DOIPtabModel;
    QTableView *DOIPtabView;
};

#endif // MESSAGEVIEW_H
