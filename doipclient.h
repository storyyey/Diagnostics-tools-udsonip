#ifndef DOIPCLIENT_H
#define DOIPCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QCheckBox>
#include <QByteArray>
#include <QDateTime>
#include <QTcpSocket>
#include <QTimer>
#include <QPlainTextEdit>
#include <QMap>
#include <QDir>
#include <QSettings>
#include <QTextCodec>
#include "messageview.h"

#define DoipHeaderLen (8)

#define Generic_DoIP_header_NACK                (0x0000)
#define vehicleIdentificationRequest            (0x0001)
#define vehicleIdentificationEIDRequest         (0x0002)
#define vehicleIdentificationVINRequest         (0x0003)
#define vehicleIdentificationResponseMessage    (0x0004)
#define routingActivationRequest                (0x0005)
#define routingActivationResponse               (0x0006)
#define aliveCheckRequest                       (0x0007)
#define aliveCheckResponse                      (0x0008)
#define DoipEntityStatusRequest                 (0x4001)
#define DoipEntityStatusResponse                (0x4002)
#define DiagPowerModeInfoRequset                (0x4003)
#define DiagPowerModeInfoResponse               (0x4004)
#define diagnosticMessage                       (0x8001)
#define diagnosticMessageAck                    (0x8002)
#define diagnosticMessageNack                   (0x8003)

typedef struct DoipHeader_s {
    uint8_t version;
    uint8_t inverse_version;
    uint16_t payload_type;
    uint32_t payload_length;
} DoipHeader_t;

typedef struct frameStream_s {
    uint8_t *buff;
    uint32_t bufflen;
    uint32_t msglen;
} frameStream_t;

typedef struct VehicleIdentificationInfo_s {
    QHostAddress address;
    char vin[17];
    quint16 addr;
    char eid[6];
    char gid[6];
} VehicleIdentificationInfo_t;

class DoipClientConnect : public QObject
{
    Q_OBJECT
public:
    enum sendDiagFinishState{
        normalFinish,
        TimeoutFinish,
        nackFinish,
    };
    explicit DoipClientConnect(QObject *parent = nullptr);
    DoipClientConnect(uint32_t recvlen, uint32_t sendlen);
    ~ DoipClientConnect();
    void backupCacheInit();
    void operationCache();
    bool routeActiveRequest(QHostAddress hostAddr, quint16 hostPort, quint8 version, quint16 sourceAddr, quint16 timeout = 2000);
    quint32 diagnosisRequest(const char *buf, quint32 len, quint16 ta, bool supression = false, quint16 timeout = 2000);
    QHostAddress getServerHostAddress();
    quint16 getServerHostPort();
    quint8 getDoipVersion();
    quint16 getSourceAddress();
    quint16 getFuncTargetAddress();
    quint16 getLogicTargetAddress();
    QString getServerVin();
    QString getName();
    void setReconnect(bool reconnect);
    bool isReconnect();
    bool setServerVin(QString vin);
    bool isRouteActive();
    void disconnect();
    QString serviceBackCacheDir = QDir::currentPath() + "/backcache/";
    qint32 upAverageByte = 0;
    qint32 downAverageByte = 0;
    static int transferAverageSpeed(QDateTime &preTime, qint32 &totalByte, qint32 &averageByte);
private slots:

signals:
    void diagnosisFinish(DoipClientConnect *doipConn, DoipClientConnect::sendDiagFinishState state, QByteArray &response);
    void routeActiveStateChange(DoipClientConnect *conn, bool state);
    void newDoipMessage(const doipMessage_t &val);
private:
    QString name;
    QHostAddress hostAddr;
    quint16 hostPort;
    quint8 version;
    quint8 activeSid;
    quint16 sourceAddr;
    quint16 funcTargetAddr;
    quint16 logicTargetAddr;
    quint8 iso[4];
    quint8 oem[4];
    QString vin;
    QTcpSocket *TcpSocket;
    QTimer *diagMsgTimer;
    QTimer *activeTimer;
    QTime sendDiagTime;
    bool clientIsActive;
    frameStream_t sendStream = {nullptr, 0, 0};
    frameStream_t recvStream = {nullptr, 0, 0};
    bool isSupression;
    bool reconnect;
    bool slotIsConnect;

    QDateTime upPreTime = QDateTime::currentDateTime();;
    qint32 upTotalByte = 0;

    QDateTime downPreTime = QDateTime::currentDateTime();;
    qint32 downTotalByte = 0;
};

class DoipClient : public QObject
{
    Q_OBJECT
public:
    explicit DoipClient(QObject *parent = nullptr);
    QWidget *DoipWidget();
    void backupCacheInit();
    void operationCache();
    static bool DoipHeaderEncode(frameStream_t &stream, DoipHeader_t &header);
    static bool DoipHeaderDecode(frameStream_t &stream, DoipHeader_t &header);
    static bool DoipPayloadEncode(frameStream_t &stream, const char *payload, uint32_t payload_length);
    static bool DoipVehicleIdResponseDecode(frameStream_t &stream, VehicleIdentificationInfo_t &vid);
    int sendDoipMessage(QUdpSocket *udpSocket, const QHostAddress &host, quint16 port, DoipHeader_t &header, const char *payload, uint32_t payload_length);
    int sendDoipMessage(const QHostAddress &host, quint16 port, DoipHeader_t &header, const char *payload, uint32_t payload_length);
    static QString DoipMessageType(qint32 type);
    QMap<QString, DoipClientConnect *> getDoipServerList();
    QString serviceBackCacheDir = QDir::currentPath() + "/backcache/";
signals:
   void newDoipMessage(const doipMessage_t &val);
   void doipServerChange(QMap<QString, DoipClientConnect *> &map);
   void diagnosisFinishSignal(DoipClientConnect *doipConn, DoipClientConnect::sendDiagFinishState state, QByteArray &response);
private:
    QUdpSocket *udpSocket = nullptr;
    frameStream_t sendStream;
    frameStream_t recvStream;
    QPushButton *vehicleIdReqSendBtn = nullptr;
    QPushButton *vehicleActiveSendBtn = nullptr;
    QPushButton *newConBtn = nullptr;
    QPushButton *diagSendBtn = nullptr;
    QStandardItemModel *vehicleInfoModel = nullptr;
    QTableView *doipConnInfoView = nullptr;
    QStandardItemModel *doipConnInfoModel = nullptr;
    QTimer *speedTimer = nullptr;
    bool doipClientConnInfoIsChange;

    QLineEdit *vehicleIdTimeoutEdit = nullptr;
    QLabel *vehicleIdResponseLabel = nullptr;
    QTimer *vehicleIdRespTimer = nullptr;
    QTime  vehicleIdReqTime;

    QLineEdit *doipConnTimeoutEdit = nullptr;
    QLabel *doipConnResponseLabel = nullptr;
    QTimer *doipConnRespTimer = nullptr;
    QTime  doipConnReqTime;

    QTimer *doipConnScanTimer = nullptr;

    DoipClientConnect *newDoipClientConn = nullptr;
    DoipClientConnect *udsDoipClientConn = nullptr;
    QList<DoipClientConnect *> doipClientConnectInfos;
    QList<VehicleIdentificationInfo_t> vehicleIdentificationInfos;

    QComboBox *DoipClientListBox = nullptr;
    QPlainTextEdit *diagRespMsgEdit = nullptr;

    QPushButton* sendDoipBtn = nullptr;
    QComboBox *doipVerComBox = nullptr;
    QComboBox *srvAddrComBox = nullptr;
    QComboBox *srvPortComBox = nullptr;
    QComboBox *vinComBox = nullptr;
    QComboBox *EIDComBox = nullptr;
    QComboBox *phyAddrComBox = nullptr;
    QComboBox *funcAddrComBox = nullptr;
    QComboBox *saAddrComBox = nullptr;
    QComboBox *clientAddrComBox = nullptr;
    QComboBox *clientPortComBox = nullptr;
    QPushButton *createDoipClient = nullptr;
    QPushButton *destroyDoipClient = nullptr;
    QLineEdit *recvBuffLineEdit = nullptr;
    QLineEdit *sendBuffLineEdit = nullptr;

    QString btnCommonStyle = " QPushButton {\
                                background-color: #FFFFFF;\
                                color: #000000;\
                                padding:4px;\
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
private slots:
    void readPendingDatagrams();
    void addDoipMessage(const doipMessage_t &val);
    void RouteActiveState(DoipClientConnect *conn, bool state);
    void diagnosisResponse(DoipClientConnect *doipConn, DoipClientConnect::sendDiagFinishState state, QByteArray &response);
};

#endif // DOIPCLIENT_H
