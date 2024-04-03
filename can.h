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
#ifdef __HAVE_CAN_ZLG__
#include "zlgcan.h"
#endif /* __HAVE_CAN_ZLG__ */

class CAN : public QThread
{
    Q_OBJECT
public:
    explicit CAN(QObject *parent = nullptr);
    QWidget *startWidget();
    QWidget *deviceManageWidget();
#ifdef __HAVE_CAN_ZLG__
    void AddData(const ZCAN_Receive_Data* data, UINT len);
    void AddData(const ZCAN_ReceiveFD_Data* data, UINT len);
#endif /* __HAVE_CAN_ZLG__ */
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
#ifdef __HAVE_CAN_ZLG__
        canid_t requestId = 0;
        canid_t responseId = 0;
#endif /* __HAVE_CAN_ZLG__ */
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
#ifdef __HAVE_CAN_ZLG__
    DEVICE_HANDLE dhandle = INVALID_DEVICE_HANDLE;
    CHANNEL_HANDLE chHandle = INVALID_CHANNEL_HANDLE;
#endif /* __HAVE_CAN_ZLG__ */
    bool channelActive = false;
    int channelNumber = 0;
    quint8 activeSid;
#ifdef __HAVE_CAN_ZLG__
    QMap<QString, CHANNEL_HANDLE> channelMap;
#endif /* __HAVE_CAN_ZLG__ */
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
