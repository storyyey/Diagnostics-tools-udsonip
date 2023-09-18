#ifndef CAN_H
#define CAN_H

#include <QObject>
#include <QWidget>
#include <QLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QStringList>
#include <QLineEdit>
#include <QTimer>
#include <QThread>
#include <QQueue>
#include <QMap>
#include "zlgcan.h"

class CAN : public QThread
{
    Q_OBJECT
public:
    explicit CAN(QObject *parent = nullptr);
    QWidget *startWidget();
    QWidget *deviceManageWidget();
    void AddData(const ZCAN_Receive_Data* data, UINT len);
    void AddData(const ZCAN_ReceiveFD_Data* data, UINT len);
    void run();
    bool sendRequest(const char *buf, quint32 len, quint32 sa, quint32 ta, bool supression, quint16 timeout);
    bool sendFrames(int frames, quint16 delay);
    bool responseHandle(unsigned char *buf, quint32 len);
    QString getCurrChannel();
    QStringList getChannelList();
    enum sendDiagFinishState{
        normalFinish,
        TimeoutFinish,
        nackFinish,
    };
    struct requestHandler {
        canid_t requestId = 0;
        canid_t responseId = 0;
        bool supression = false;
        quint16 timeout = 0;
        QByteArray firstFram;

        QQueue<QByteArray> frams;
    };

    struct responseHandler {
        int expectLen;
        QByteArray responseFram;
    };
private:
    DEVICE_HANDLE dhandle = INVALID_DEVICE_HANDLE;
    CHANNEL_HANDLE chHandle = INVALID_CHANNEL_HANDLE;
    bool channelActive = false;
    int channelNumber = 0;
    quint8 activeSid;
    QMap<QString, CHANNEL_HANDLE> channelMap;
    QMap<QString, int> canfd_abit_baud_rateMap;
    QMap<QString, int> canfd_dbit_baud_rateMap;
    QWidget *canchannelStartWidget = nullptr;
    QWidget *devicesManageWidget = nullptr;
    QPushButton *cancelBtn = nullptr;
    QPushButton *confirmBtn = nullptr;
    QComboBox *terResistorComBox = nullptr;
    QComboBox *workModelComBox = nullptr;
    QLineEdit *customLineEdit = nullptr;
    QComboBox *dataAudRateComBox = nullptr;
    QComboBox *audRateComBox = nullptr;
    QComboBox *canfdAddSpeedComBox = nullptr;
    QComboBox *canfdStandComBox = nullptr;
    QComboBox *protocolComBox = nullptr;
    QTimer *diagTimer = nullptr;
    QComboBox *deviceTypeComBox = nullptr;
    QComboBox *canchannelComBox = nullptr;
    QPushButton *usbCanfd200uChannel0StartBtn = nullptr;
    QPushButton *usbCanfd200uChannel1StartBtn = nullptr;
    QPushButton *usbCanfd200uChannel0StopBtn = nullptr;
    QPushButton *usbCanfd200uChannel1StopBtn = nullptr;

    requestHandler requestHandler;
    responseHandler responseHandler;
signals:
    void udsResponse(QString name, int, QByteArray response);
};

#endif // CAN_H
