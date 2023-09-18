#ifndef UDS_H
#define UDS_H

#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QPushButton>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QHorizontalPercentBarSeries>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QProgressBar>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QDirIterator>
#include <QListWidget>
#include <QQueue>
#include <QElapsedTimer>
#include <QProcess>
#include <QTextCodec>
#include <QMessageBox>
#include <QLibrary>
#include <QSettings>
#include <QStackedWidget>
// #include <QMovie>
#include <QMenu>
#include <QLCDNumber>
#include <QCryptographicHash>
#include <QApplication>
#include <QClipboard>
#include "fileselect.h"
#include "can.h"
#include "doipclient.h"
#include "tabletoxlsx.h"

QT_CHARTS_USE_NAMESPACE

void realTimeLog(QString log);

class serviceItem
{
public:
    quint8 SID = 0;
    quint8 SUB = 0;
    quint16 did = 0;
    quint32 ta = 0;
    QString tatype;
    quint16 timeout = 0;
    quint32 delay = 0;
    bool isspuress = false;
    QByteArray requestMsg;
    QString desc;
    QString expectResponse;
    QString expectResponseRule;
    QString resultDesc;

    quint8 modeOfOperation = 0;
    quint16 filePathAndNameLength = 0;
    QString filePathAndName;
    quint8 dataFormatIdentifier = 0;
    quint8 fileSizeParameterLength = 0;
    quint32 fileSizeUnCompressed = 0;
    quint32 fileSizeCompressed = 0;

    QString securityAccess;
    QString localFilePath;

    QString UDSFinishCondition;
    QString UDSFinishData;
    quint32 UDSFinishMaxNumber;
};

class UDS : public QObject
{
    Q_OBJECT
public:
    explicit UDS(QObject *parent = nullptr);
    void backupCacheInit();
    QWidget *UDSMainWidget();
    QWidget *userSelectWidget();
    QWidget *UDSChannelSelectWidget();
    QWidget *UDSOperationConfigWidget();
    bool saveServiceSet(QString filename);
    bool addServiceItem(serviceItem &data);
    bool setServiceItemOperationControl(int row, serviceItem &data);
    bool modifyServiceItem(int row, serviceItem &data);
    bool getServiceItemData(int row, serviceItem &data);
    QJsonDocument serviceItemDataToJson(serviceItem &data);
    QString serviceItemDataEncode(serviceItem &data);
    QString serviceItemDataEncodeUnformat(serviceItem &data);
    QByteArray getRequestFileTransferMessage(serviceItem &data);
    serviceItem serviceItemDataDecode(QString &config);
    QWidget *serviceConfigWidget();
    QWidget *requestFileTransferWidget();
    QWidget *SecurityAccessWidget();
    QByteArray serviceRequestEncode();
    QString widgetConfigToString();
    bool stringToWidgetConfig(QString &config);
    void setTableItemColor(QStandardItemModel *model, int index, QColor &color);
    void setTableItemColor(QStandardItemModel *model, int index, const char *color);
    bool serviceActive();
    void serviceRequestHandler(serviceItem &data);
    bool sendServiceRequest(int index, QString channelName);
    void serviceListReset(QStandardItemModel *model);
    void visualizationTestResult();
    bool abortServiceTask();
    QWidget *serviceProjectListWidget();
    bool loadServiceSet(QString filename);
    bool serviceSetActive();
    void promptForAction(QString text);
    bool serviceResponseHandle(QByteArray response, serviceItem &data);
    bool keepCurrentTask();
    void udsViewSetColumnHidden(bool hidden);
    bool createAdbProcess();
    bool execAdbCmd(QStringList args);
    bool adbDiagRequest(const char *buf, quint32 len, quint16 ta, bool supression = false, quint16 timeout = 2000);
    QString udsParse(QByteArray msg);
    bool addDtcDictionaryItem(QString text);
    bool addDidDictionaryItem(QString text);
    QString generateTestResultFileName();
    bool serviceResponseToTable(int indexRow, QByteArray response);
    bool serviceSetDelItem(int row);
    bool serviceSetChangeItem(int row);
    bool serviceSetMoveItem(int row, int newrow);
    bool projectSetMoveItem(int row, int newrow);
    static int transferAverageSpeed(QDateTime &preTime, qint32 &totalByte, qint32 &averageByte);
    void serverSetFinishHandle();
    enum serviceTaskState {
        WaiteSend,
        SendTask,
        WaiteRecv,
        RecvTask,
    };
    enum serviceResponseExpect {
        notSetResponseExpect = 1,
        positiveResponseExpect,
        negativeResponseExpect,
        noResponseExpect,
        responseRegexMatch,
        responseRegexMismatch,
    };
    enum serviceItemState {
        serviceItemAll,
        serviceItemNotTest,
        serviceItemTestPass,
        serviceItemTestFail,
        serviceItemTestWarn,
        serviceItemTestProcess,
    };
    enum serviceItemState serverSetItemResultState(serviceItem data, QByteArray response);
    class UDSServiceTask {
        public:
            int indexRow = -1;
            serviceItem data;
            QString channelName;
            int cyclenum = 0;
            quint32 runtotal = 0;
            QTimer responseTimer; /* 响应超时定时器 */
            QDateTime upPreTime;
            qint32 upAverageByte;
            qint32 upTotalByte;

            QDateTime downPreTime;
            qint32 downAverageByte;
            qint32 downTotalByte;
    };
    enum modeOfOperation {
        AddFile = 0x01,
        DeleteFile = 0x02,
        ReplaceFile = 0x03,
        ReadFile = 0x04,
        ReadDir = 0x05,
    };
    enum UDSFinishCondition {
        UDSFinishDefaultSetting = 0x00,
        UDSFinishEqualTo,
        UDSFinishUnEqualTo,
        UDSFinishRegexMatch,
        UDSFinishRegexMismatch,
        UDSFinishRegularExpressionMatch,
    };
    enum {
        reportDTCByStatusMask = 0x02,
        reportSupportedDTCs = 0x0A,
        reportFirstTestFailedDTC = 0x0B,
        reportFirstConfirmedDTC = 0x0C,
        reportMostRecentTestFailedDTC = 0x0D,
        reportMostRecentConfirmedDTC = 0x0E,
        reportMirrorMemoryDTCByStatusMask = 0x0F,
        reportEmissionsOBDDTCByStatusMask = 0x13,
        reportDTCWithPermanentStatus = 0x15,
    };

    enum {
        DiagnosticSessionControl = 0x10,
        ECUReset = 0x11,
        SecurityAccess = 0x27,
        CommunicationControl = 0x28,
        TesterPresent = 0x3e,
        AccessTimingParameter = 0x83,
        SecuredDataTransmission = 0x84,
        ControlDTCSetting = 0x85,
        ResponseOnEvent = 0x86,
        LinkControl = 0x87,
        ReadDataByIdentifier = 0x22,
        ReadMemoryByAddress = 0x23,
        ReadScalingDataByIdentifier = 0x24,
        ReadDataByPeriodicIdentifier = 0x2A,
        DynamicallyDefineDataIdentifier = 0x2C,
        WriteDataByIdentifier = 0x2E,
        WriteMemoryByAddress = 0x3D,
        ClearDiagnosticInformation = 0x14,
        ReadDTCInformation = 0x19,
        InputOutputControlByIdentifier = 0x2F,
        RoutineControl = 0x31,
        RequestDownload = 0x34,
        RequestUpload = 0x35,
        TransferData = 0x36,
        RequestTransferExit = 0x37,
        RequestFileTransfer = 0x38,
    };
    QString serviceSetProjectDir = QDir::currentPath() + "/serviceTestProject/";
    QString securityDllDir = QDir::currentPath() + "/securityDll/";
    QString serviceProjectListConfigDir = QDir::currentPath() + "/serviceProjectListConfig/";
    QString serviceBackCacheDir = QDir::currentPath() + "/backcache/";
    QString serviceTestResultDir = QDir::currentPath() + "/testResult/";
    QString servicetransferFileDir = serviceSetProjectDir + "transferFile/";
signals:

private:
    QCheckBox *highPerformanceCheckBox = nullptr;
    bool highPerformanceEnable = false; /* 高性能模式使能将减少UI动画交互 */
    CAN *can = nullptr;
    int fontHeight = 17;

    QComboBox *projectNameListComBox = nullptr;

    TableToXlsx tableToXlsx;
    QString resultFileName;
    QCheckBox *testResultCheckBox = nullptr;

    QTimer *serviceSetBackTimer = nullptr;

    QMap<QString, QString> dtcDescDictionary;
    QMap<QString, QString> didDescDictionary;

    QComboBox *adbLoginHintEdit = nullptr;
    QComboBox *adbLoginNameEdit = nullptr;
    QComboBox *adbPasswordHintEdit = nullptr;
    QComboBox *adbPasswordEdit = nullptr;
    QLabel *adbStateLabel = nullptr;

    QWidget *doipChannelConfigWidget = nullptr;
    QWidget *canChannelConfigWidget = nullptr;
    QWidget *adbChannelConfigWidget= nullptr;

    QString adbDiagChannelName = "UDSonADB";
    QLineEdit *canRequestAddr = nullptr;
    QLineEdit *canResponseFuncAddr = nullptr;
    QLineEdit *canResponsePhysAddr = nullptr;
    QCheckBox *doipDiagCheckBox = nullptr;
    QLineEdit *doipResponseFuncAddr = nullptr;
    QLineEdit *doipResponsePhysAddr = nullptr;
    QCheckBox *canDiagCheckBox = nullptr;
    QCheckBox *adbDiagCheckBox = nullptr;
    QProcess *adbListenProcess = nullptr;
    QStringList adbListenExecArgs;
    QProcess *adbCmdExecProcess = nullptr;
    QTimer *adbListenTimer = nullptr;
    QTimer *adbCmdTimeoutTimer = nullptr;
    struct adbDiagTask {
        bool isvalid = false;
        bool issuppress = false;
        QStringList adbWaitExecArgs;
        quint16 adbCmdTimeoutInterval;
    };
    adbDiagTask adbWaitTask;
    adbDiagTask adbActiveTask;

    int udsViewItemEnable = 0;
    int udsViewRequestColumn = 1;
    int udsViewSupressColumn = 2;
    int udsViewTimeoutColumn = 3;
    int udsViewDelayColumn = 4;
    int udsViewElapsedTimeColumn = 5;
    int udsViewResponseColumn = 6;
    int udsViewExpectResponseColumn = 7;
    int udsViewRemarkColumn = 8;
    int udsViewSendBtnColumn = 9;
    int udsViewConfigBtnColumn = 10;
    int udsViewUpBtnColumn = 11;
    int udsViewDownBtnColumn = 12;
    int udsViewDelBtnColumn = 13;
    int udsViewResultColumn = 14;

    QLCDNumber *allServiceElapsedTimeLCDNumber = nullptr;
    QElapsedTimer allServiceTimer;
    QLCDNumber *serviceElapsedTimeLCDNumber = nullptr;
    QElapsedTimer serviceTimer;
    QLabel *promptActionLabel = nullptr;
    QProgressBar *promptActionProgress = nullptr;
    QTimer *serviceListTaskTimer = nullptr;
    UDS::serviceTaskState currTaskState;
    UDSServiceTask serviceActiveTask;

    QMap<QString, quint8> serviceMap;
    QMap<quint8, QMap<QString, quint8>> serviceSubMap;
    QMap<quint8, QString> serviceDescMap;
    QMap<quint8, QMap<quint8, QString>> serviceSubDescMap;
    QMap<QString, UDS::serviceResponseExpect> responseExpectMap;
    QMap<UDS::serviceResponseExpect, QString> responseExpectDesc;
    QMap<QString, quint8> positiveResponseCodeMap;
    QMap<quint8, QString> positiveResponseCodeDesc;

    QComboBox *TestProjectNameEdit = nullptr;
    QCheckBox *UDSDescolumnCheckBox = nullptr;
    QCheckBox *SendBtnCheckBox = nullptr;
    QCheckBox *ConfigBtnCheckBox = nullptr;
    QCheckBox *UpBtnCheckBox = nullptr;
    QCheckBox *DownBtnCheckBox = nullptr;
    QCheckBox *DelBtnCheckBox = nullptr;

    QPushButton *addServiceBtn = nullptr;
    QPushButton *addDidDescBtn = nullptr;
    QPushButton *addDtcDescBtn = nullptr;
    QCheckBox *serviceTestResultCheckBox = nullptr;
    QStringList serviceSetTabHeaderTitle;
    QStandardItemModel *serviceSetModel = nullptr;
    int serviceSetSelectRow = -1;
    int serviceSetSelectCloumn = -1;
    QMenu *serviceSetMenu = nullptr;
    QTableView *serviceSetView = nullptr;
    QWidget *serviceSetWidget = nullptr;

    QComboBox *doipServerListBox = nullptr;

    QLineEdit *cycleNumberEdit = nullptr;
    QComboBox *serviceListBox = nullptr;
    QComboBox *serviceSubListBox = nullptr;
    QLineEdit *serviceEidt = nullptr;
    QLineEdit *serviceSubEidt = nullptr;
    QLineEdit *serviceDidEidt = nullptr;
    QCheckBox *serviceSuppressCheck = nullptr;
    QLineEdit *serviceTimeoutEdit = nullptr;
    QLineEdit *serviceDelayEdit = nullptr;
    QLineEdit *serviceDesc = nullptr;
    QLineEdit *serviceTaEdit = nullptr;
    QComboBox *serviceAddrType = nullptr;
    QPlainTextEdit *diagReqMsgEdit = nullptr;
    QPushButton *udsRequestConfigBtn = nullptr;
    QComboBox *UDSConditionRuleComboBox = nullptr;
    QLineEdit *UDSFinishRuleEdit = nullptr;
    QPlainTextEdit *UDSFinishPlainText = nullptr;
    QMap<QString, UDS::UDSFinishCondition> UDSConditionMap;
    QMap<UDS::UDSFinishCondition, QString> UDSConditionMapDesc;
    QCheckBox *UDSResponseParseCheckBox = nullptr;

    QPushButton *serviceSetResetBtn = nullptr;
    QPushButton *serviceSetclearBtn = nullptr;
    QPushButton *saveTestProjectBtn = nullptr;
    QPushButton *delTestProjectBtn = nullptr;

    quint32 transfer36Maxlen = 0;
    QLineEdit *transfer36MaxlenEdit = nullptr;

    QTimer *cycle3eTimer = nullptr;
    QLineEdit *cycle3eEdit = nullptr;
    QLineEdit *taAdress3eEdit = nullptr;
    QLineEdit *saAdress3eEdit = nullptr;
    QCheckBox *suppress3eCheckBox = nullptr;
    QCheckBox *fixCycle3eCheckBox = nullptr;

    QWidget *fileTransferWidget = nullptr;
    QWidget *securityAccessWidget = nullptr;

    QPushButton *sendListBtn = nullptr;
    QPushButton *sendAbortListBtn = nullptr;

    QComboBox *serviceExpectRespBox = nullptr;
    QComboBox *negativeResponseCodeBox = nullptr;
    QPlainTextEdit *serviceRespPlainText = nullptr;

    int modifyRowIndex;
    int manualSendRowIndex;
    QMap<QString, DoipClientConnect *> doipConnMap;

    QWidget *progressWidget = nullptr;
    QHBoxLayout *progressLayout = nullptr;
    QProgressBar *serviceTestProgress = nullptr;
    QBarCategoryAxis *axisY = nullptr;
    QValueAxis *axisX = nullptr;
    QChart *chart = nullptr;
    QChartView *chartView = nullptr;
    QLabel *upTransferSpeedLable = nullptr;
    QLabel *downTransferSpeedLable = nullptr;
    QMap<UDS::serviceItemState, int> serviceItemStatis;
    QMap<UDS::serviceItemState, QLabel *> serviceItemStatisLabel;
    QMap<UDS::serviceItemState, const char *> serviceItemStateColor;
    QString lineEditStyle = "QLineEdit {border: 1px solid #8B8378;border-radius: 2px}\
                             QLineEdit:focus{border: 1px solid #63B8FF;border-radius: 2px}";
    QString lineEditHintStyle = "QLineEdit{border-style: solid; border-width: 1px; border-color: red;}";
    QString pushButtonStyle = "";//".QPushButton:hover {background-color: #BDBDBD;border-radius: 2px;}\
                              //              .QPushButton:pressed {background-color: #8F8F8F;border-radius: 2px;}\
                              //              .QPushButton {background-color:#D3D3D3;height:20px;border-radius: 2px;border: 1px solid #8B8378}";

    QComboBox *projectNameComBox = nullptr;
    QWidget *ProjectListWidget = nullptr;
    QTimer *projectListTimer = nullptr;
    int projectListIndex = -1;
    QTableView *projectNameTable = nullptr;
    QMenu *projectNameWidgetMenu = nullptr;
    int projectSetSelectRow = -1;
    int projectSetSelectCloumn = -1;
    QStandardItemModel *projectNameModel = nullptr;
    QPushButton *loadTestProjectBtn = nullptr;

    QComboBox *modeOfOperationComBox = nullptr;
    QLineEdit *filePathAndNameLengthEdit = nullptr;
    QLineEdit *filePathAndNameEdit = nullptr;
    QLineEdit *dataFormatIdentifierEdit = nullptr;
    QLineEdit *fileSizeParameterLengthEdit = nullptr;
    QLineEdit *fileSizeUnCompressedEdit = nullptr;
    QLineEdit *fileSizeCompressedEdit = nullptr;
    QString fileTransferPath;

    QMap<QString, quint8> modeOfOperationMap;
    QMap<quint8, QString> modeOfOperationDescMap;

    QComboBox *SecurityAccessComBox = nullptr;

    QByteArray securityAccessSeed;
    QByteArray securityAccessKey;

    quint8 modeOfOperation = 0;
    quint8 lengthFormatIdentifier = 0;
    quint32 maxNumberOfBlockLength = 0;
    quint16 dataFormatIdentifier = 0;
    quint8 fileTransferCount = 0;
    quint32 fileTransferTotalSize = 0;
    QString localFilePath;

    bool iskeepCurrentTask = false;
    QFile transferFile;

    struct flickerItem {
        bool isvalid;
        bool direct;
        int statusLevel;
        QList<QStandardItem *> Item;
    };
    struct flickerItem activeServiceFlicker;
    struct flickerItem activeServiceProjectFlicker;
    void setActiveServiceFlicker(QList<QStandardItem *> &item);
    void setActiveServiceProjectFlicker(QList<QStandardItem *> &item);

    QString btnCommonStyle = " QPushButton {\
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
    QString comboxCommonStyle;

public slots:
    void doipServerListChangeSlot(QMap<QString, DoipClientConnect *> &map);
    void diagnosisResponseSlot(DoipClientConnect *doipConn, DoipClientConnect::sendDiagFinishState state, QByteArray &response);
    void diagnosisResponseHandle(QString channelName, DoipClientConnect::sendDiagFinishState state, QByteArray &response);
    void serviceSetBackupCache();
    void operationCache();
    void canResponseHandler(QString name, int, QByteArray response);
    void contextMenuSlot(QPoint pos);
    void projectSetContextMenuSlot(QPoint pos);
    void serviceSetDelMenuSlot();
};

#endif // UDS_H
