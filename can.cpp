#include <QLibrary>
#include <QDebug>
#include "can.h"

CAN::CAN(QObject *parent) : QThread(parent)
{
    devicesManageWidget = new QWidget();
    QVBoxLayout *deviceManageMainLayout = new QVBoxLayout();
    devicesManageWidget->setLayout(deviceManageMainLayout);

    QHBoxLayout *deviceSelectLayout = new QHBoxLayout();
    deviceManageMainLayout->addLayout(deviceSelectLayout);
    deviceSelectLayout->addWidget(new QLabel("类型:"));
    deviceTypeComBox = new QComboBox();
    deviceTypeComBox->addItems(QStringList() << "USBCANFD-200U" << "USBCANFD-100U");
    deviceSelectLayout->addWidget(deviceTypeComBox);
    deviceSelectLayout->addWidget(new QLabel("索引:"));
    QComboBox *deviceIndexComBox = new QComboBox();
    deviceIndexComBox->addItems(QStringList() << "0" << "1" << "2");
    deviceSelectLayout->addWidget(deviceIndexComBox);
    QPushButton *openDeviceBtn = new QPushButton("打开设备");
    deviceSelectLayout->addWidget(openDeviceBtn);

    QWidget *usbCanfd200uWidget = new QWidget();
    usbCanfd200uWidget->hide();
    QGridLayout *usbCanfd200uMainLayout = new QGridLayout();
    deviceManageMainLayout->addWidget(usbCanfd200uWidget);
    usbCanfd200uWidget->setLayout(usbCanfd200uMainLayout);

    usbCanfd200uMainLayout->addWidget(new QLabel(deviceTypeComBox->currentText() + " 设备" + deviceIndexComBox->currentText()), 0, 0);
    QPushButton *usbCanfd200uStartBtn = new QPushButton("启动");
    usbCanfd200uMainLayout->addWidget(usbCanfd200uStartBtn, 0, 1);
    QPushButton *usbCanfd200uStopBtn = new QPushButton("停止");
    usbCanfd200uMainLayout->addWidget(usbCanfd200uStopBtn, 0, 2);
    QPushButton *usbCanfd200uCloseBtn = new QPushButton("关闭设备");
    usbCanfd200uMainLayout->addWidget(usbCanfd200uCloseBtn, 0, 3);

    usbCanfd200uMainLayout->addWidget(new QLabel("通道0"), 1, 0);
    usbCanfd200uChannel0StartBtn = new QPushButton("启动");
    usbCanfd200uMainLayout->addWidget(usbCanfd200uChannel0StartBtn, 1, 1);
    usbCanfd200uChannel0StopBtn = new QPushButton("停止");
    usbCanfd200uMainLayout->addWidget(usbCanfd200uChannel0StopBtn, 1, 2);

    usbCanfd200uMainLayout->addWidget(new QLabel("通道1"), 2, 0);
    usbCanfd200uChannel1StartBtn = new QPushButton("启动");
    usbCanfd200uMainLayout->addWidget(usbCanfd200uChannel1StartBtn, 2, 1);
    usbCanfd200uChannel1StopBtn = new QPushButton("停止");
    usbCanfd200uMainLayout->addWidget(usbCanfd200uChannel1StopBtn, 2, 2);

    connect(openDeviceBtn, &QPushButton::clicked, this, [this, usbCanfd200uWidget, deviceIndexComBox](){
#ifdef __HAVE_CAN_ZLG__
        dhandle = ZCAN_OpenDevice(ZCAN_USBCANFD_200U, deviceIndexComBox->currentText().toInt(), 0);
        if (INVALID_DEVICE_HANDLE == dhandle) {
               qDebug() << "打开设备失败" ;
               return ;
        }
        usbCanfd200uWidget->show();
#endif /* #ifdef __HAVE_CAN_ZLG__ */
    });

    connect(usbCanfd200uChannel0StartBtn, &QPushButton::clicked, this, [this](){
        channelNumber = 0;
        canchannelStartWidget->setWindowFlags(canchannelStartWidget->windowFlags() | Qt::WindowStaysOnTopHint);
        canchannelStartWidget->show();
    });

    connect(usbCanfd200uChannel1StartBtn, &QPushButton::clicked, this, [this](){
        channelNumber = 1;
        canchannelStartWidget->setWindowFlags(canchannelStartWidget->windowFlags() | Qt::WindowStaysOnTopHint);
        canchannelStartWidget->show();
    });

    connect(usbCanfd200uChannel0StopBtn, &QPushButton::clicked, this, [this](){
#ifdef __HAVE_CAN_ZLG__
        QString channelName = deviceTypeComBox->currentText() + QString("通道%1").arg(0);

        if (channelMap.contains(channelName)) {
            ZCAN_ResetCAN(channelMap[channelName]);
            channelMap.remove(channelName);
            canchannelComBox->setCurrentText(channelName);
            canchannelComBox->removeItem(canchannelComBox->currentIndex());
        }
        usbCanfd200uChannel0StartBtn->setEnabled(true);
        usbCanfd200uChannel0StopBtn->setEnabled(false);
        canchannelStartWidget->hide();
#endif /* __HAVE_CAN_ZLG__ */
    });

    connect(usbCanfd200uChannel1StopBtn, &QPushButton::clicked, this, [this](){
#ifdef __HAVE_CAN_ZLG__
        QString channelName = deviceTypeComBox->currentText() + QString("通道%1").arg(1);

        if (channelMap.contains(channelName)) {
            ZCAN_ResetCAN(channelMap[channelName]);
            channelMap.remove(channelName);
            canchannelComBox->setCurrentText(channelName);
            canchannelComBox->removeItem(canchannelComBox->currentIndex());
        }
        usbCanfd200uChannel1StartBtn->setEnabled(true);
        usbCanfd200uChannel1StopBtn->setEnabled(false);
        canchannelStartWidget->hide();
#endif /* __HAVE_CAN_ZLG__ */
    });

    connect(usbCanfd200uCloseBtn, &QPushButton::clicked, this, [this, usbCanfd200uWidget](){
#ifdef __HAVE_CAN_ZLG__
        QString channelName = deviceTypeComBox->currentText() + QString("通道%1").arg(0);
        if (channelMap.contains(channelName)) {
            ZCAN_ResetCAN(channelMap[channelName]);
            qDebug() << "ZCAN_ResetCAN " << channelName <<  channelMap[channelName];
        }
        channelName = deviceTypeComBox->currentText() + QString("通道%1").arg(1);
        if (channelMap.contains(channelName)) {
            ZCAN_ResetCAN(channelMap[channelName]);
            qDebug() << "ZCAN_ResetCAN " << channelName <<  channelMap[channelName];
        }
        canchannelComBox->clear();
        channelMap.clear();
        usbCanfd200uChannel0StartBtn->setEnabled(true);
        usbCanfd200uChannel1StartBtn->setEnabled(true);
        usbCanfd200uChannel1StopBtn->setEnabled(false);
        usbCanfd200uChannel0StopBtn->setEnabled(false);
        ZCAN_CloseDevice(dhandle);
        dhandle = INVALID_DEVICE_HANDLE;
        usbCanfd200uWidget->hide();
#endif /* __HAVE_CAN_ZLG__ */
    });

    canchannelStartWidget = startWidget();
}

QWidget *CAN::startWidget()
{
    QWidget *mainWidget = new QWidget();
    QGridLayout *mainLayout = new QGridLayout();
    mainWidget->setLayout(mainLayout);

    mainLayout->addWidget(new QLabel("协议:"), 0, 0);
    protocolComBox = new QComboBox();
    protocolComBox->addItems(QStringList() << "CAN FD" << "CAN");
    mainLayout->addWidget(protocolComBox, 0, 1);

    mainLayout->addWidget(new QLabel("CANFD标准:"), 1, 0);
    canfdStandComBox = new QComboBox();
    canfdStandComBox->addItems(QStringList() << "CAN FD ISO" << "Non-ISO");
    mainLayout->addWidget(canfdStandComBox, 1, 1);

    mainLayout->addWidget(new QLabel("CANFD加速:"), 2, 0);
    canfdAddSpeedComBox = new QComboBox();
    canfdAddSpeedComBox->addItems(QStringList() << "是" << "否");
    mainLayout->addWidget(canfdAddSpeedComBox, 2, 1);

    mainLayout->addWidget(new QLabel("仲裁域波特率:"), 3, 0);
    audRateComBox = new QComboBox();
    audRateComBox->addItems(QStringList() << "1Mbps 80%" << "800kbps 80%" <<\
                            "500kbps 80%" << "250kbps 80%" << "125kbps 80%" <<\
                            "100kbps 80%" << "50kbps 80%" << "自定义");
    canfd_abit_baud_rateMap.insert("1Mbps 80%", 1000000);
    canfd_abit_baud_rateMap.insert("800kbps 80%", 800000);
    canfd_abit_baud_rateMap.insert("500kbps 80%", 500000);
    canfd_abit_baud_rateMap.insert("250kbps 80%", 250000);
    canfd_abit_baud_rateMap.insert("125kbps 80%", 125000);
    canfd_abit_baud_rateMap.insert("100kbps 80%", 100000);
    canfd_abit_baud_rateMap.insert("50kbps 80%", 50000);
    audRateComBox->setCurrentText("500kbps 80%");
    mainLayout->addWidget(audRateComBox, 3, 1);

    mainLayout->addWidget(new QLabel("数据域波特率:"), 4, 0);
    dataAudRateComBox = new QComboBox();
    dataAudRateComBox->addItems(QStringList() << "5Mbps 75%" << "4Mbps 80%" <<\
                                "2Mbps 80%" << "1Mbps 80%" << "800kbps 80%" <<\
                            "500kbps 80%" << "250kbps 80%" << "125kbps 80%" <<\
                            "100kbps 80%");
    canfd_dbit_baud_rateMap.insert("5Mbps 75%", 5000000);
    canfd_dbit_baud_rateMap.insert("4Mbps 80%", 4000000);
    canfd_dbit_baud_rateMap.insert("2Mbps 80%", 2000000);
    canfd_dbit_baud_rateMap.insert("1Mbps 80%", 1000000);
    canfd_dbit_baud_rateMap.insert("800kbps 80%", 800000);
    canfd_dbit_baud_rateMap.insert("500kbps 80%", 500000);
    canfd_dbit_baud_rateMap.insert("250kbps 80%", 250000);
    canfd_dbit_baud_rateMap.insert("125kbps 80%", 125000);
    canfd_dbit_baud_rateMap.insert("100kbps 80%", 100000);
    dataAudRateComBox->setCurrentText("2Mbps 80%");
    mainLayout->addWidget(dataAudRateComBox, 4, 1);

    mainLayout->addWidget(new QLabel("自定义波特率:"), 5, 0);
    customLineEdit = new QLineEdit();
    customLineEdit->hide();
    mainLayout->addWidget(customLineEdit, 5, 1);

    mainLayout->addWidget(new QLabel("工作模式:"), 6, 0);
    workModelComBox = new QComboBox();
    workModelComBox->addItems(QStringList() << "正常模式" << "只听模式");
    mainLayout->addWidget(workModelComBox, 6, 1);

    mainLayout->addWidget(new QLabel("终端电阻:"), 7, 0);
    terResistorComBox = new QComboBox();
    terResistorComBox->addItems(QStringList() << "禁能" << "使能");
    mainLayout->addWidget(terResistorComBox, 7, 1);

    confirmBtn = new QPushButton("确认");
    mainLayout->addWidget(confirmBtn, 8, 0);

    cancelBtn = new QPushButton("取消");
    mainLayout->addWidget(cancelBtn, 8, 1);

    connect(confirmBtn, &QPushButton::clicked, this, [this](){
#ifdef __HAVE_CAN_ZLG__
        if (dhandle == INVALID_DEVICE_HANDLE) {
            return ;
        }
        QString channelName = deviceTypeComBox->currentText() + QString("通道%1").arg(channelNumber);
        if (channelMap.contains(channelName)) {
            return ;
        }

        char path[50] = { 0 };
        sprintf_s(path, "%d/canfd_abit_baud_rate", channelNumber);
        char value[10] = { 0 };
        sprintf_s(value, "%d", canfd_abit_baud_rateMap[audRateComBox->currentText()]);
        ZCAN_SetValue(dhandle, path, value);
        sprintf_s(path, "%d/canfd_dbit_baud_rate", channelNumber);
        sprintf_s(value, "%d", canfd_dbit_baud_rateMap[dataAudRateComBox->currentText()]);
        ZCAN_SetValue(dhandle, path, value);

        ZCAN_CHANNEL_INIT_CONFIG cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.can_type = protocolComBox->currentText() == "CAN FD" ? TYPE_CANFD : TYPE_CAN;//CANFD设备为TYPE_CANFD
        cfg.can.filter = 0;
        cfg.can.mode = workModelComBox->currentText() == "正常模式" ? 0 : 1;; //正常模式, 1为只听模式
        cfg.can.acc_code = 0;
        cfg.can.acc_mask = 0xffffffff;
        chHandle = INVALID_CHANNEL_HANDLE;
        chHandle = ZCAN_InitCAN(dhandle, channelNumber, &cfg);
        if (INVALID_CHANNEL_HANDLE == chHandle) {
            qDebug() << "初始化通道失败";
        }
        if (ZCAN_StartCAN(chHandle) != STATUS_OK) {
            qDebug() << "启动通道失败";
        }
        qDebug() << "启动通道成功";
        canchannelStartWidget->hide();
        channelMap.insert(channelName, chHandle);
        canchannelComBox->addItem(channelName);
        if (channelNumber == 0) {
            usbCanfd200uChannel0StartBtn->setEnabled(false);
            usbCanfd200uChannel0StopBtn->setEnabled(true);
        }
        if (channelNumber == 1) {
            usbCanfd200uChannel1StartBtn->setEnabled(false);
            usbCanfd200uChannel1StopBtn->setEnabled(true);
        }
        this->start();
 #endif /* __HAVE_CAN_ZLG__ */
    });

    connect(cancelBtn, &QPushButton::clicked, this, [this](){
        canchannelStartWidget->hide();
    });

    return mainWidget;
}

QWidget *CAN::deviceManageWidget()
{
    QWidget *mainWidget = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainWidget->setLayout(mainLayout);

    mainLayout->addWidget(new QLabel("CAN通道:"));
    canchannelComBox = new QComboBox();
    canchannelComBox->setMinimumWidth(200);
    mainLayout->addWidget(canchannelComBox);

    QPushButton *opendeviceBtn = new QPushButton("设备管理");
    mainLayout->addWidget(opendeviceBtn);
    connect(opendeviceBtn, &QPushButton::clicked, this, [this](){
        devicesManageWidget->setWindowFlags(devicesManageWidget->windowFlags() | Qt::WindowStaysOnTopHint);
        devicesManageWidget->show();
    });

    return mainWidget;
}
#ifdef __HAVE_CAN_ZLG__
#define TMP_BUFFER_LEN 1000
void CAN::AddData(const ZCAN_Receive_Data* data, UINT len)
{
    char item[TMP_BUFFER_LEN] = {0};

    for (UINT i = 0; i < len; ++i) {
        const ZCAN_Receive_Data& can = data[i];
        const canid_t& id = can.frame.can_id;
        sprintf_s(item, "接收到CAN ID:%08X %s %s 长度:%d 数据:", GET_ID(id), IS_EFF(id)?"扩展帧":"标准帧"
            , IS_RTR(id)?"远程帧":"数据帧", can.frame.can_dlc);
        for (UINT i = 0; i < can.frame.can_dlc; ++i)
        {
            size_t item_len = strlen(item);
            sprintf_s(&item[item_len], TMP_BUFFER_LEN-item_len, "%02X ", can.frame.data[i]);
        }
        qDebug() << QString(item);
        //AddData(CString(item));
    }
}
void CAN::AddData(const ZCAN_ReceiveFD_Data* data, UINT len)
{
    char item[TMP_BUFFER_LEN] = {0};

    for (UINT i = 0; i < len; ++i) {
        const ZCAN_ReceiveFD_Data& canfd = data[i];
        const canid_t& id = canfd.frame.can_id;
        sprintf_s(item, "接收到CANFD ID:%08X %s %s 长度:%d 数据:", GET_ID(id), IS_EFF(id)?"扩展帧":"标准帧"
            , IS_RTR(id)?"远程帧":"数据帧", canfd.frame.len);
        for (UINT i = 0; i < canfd.frame.len; ++i)
        {
            size_t item_len = strlen(item);
            sprintf_s(&item[item_len], TMP_BUFFER_LEN-item_len, "%02X ", canfd.frame.data[i]);
        }
        qDebug() << QString(item);
        //AddData(CString(item));
    }
}
#endif /* __HAVE_CAN_ZLG__ */
void CAN::run()
{
#ifdef __HAVE_CAN_ZLG__
    while (dhandle != INVALID_DEVICE_HANDLE) {
        ZCAN_Receive_Data can_data[100];
        ZCAN_ReceiveFD_Data canfd_data[100];
        UINT len;
        usleep(1);
        if ((len = ZCAN_GetReceiveNum(chHandle, TYPE_CAN))) {
            len = ZCAN_Receive(chHandle, can_data, 100, 50);
            if (channelActive) {
                char item[TMP_BUFFER_LEN] = {0};

                for (UINT i = 0; i < len; ++i) {
                    const ZCAN_Receive_Data& can = can_data[i];
                    const canid_t& id = can.frame.can_id;
                    if (can.frame.can_id == requestHandler.responseId) {
                        sprintf_s(item, "接收到CAN ID:%08X %s %s 长度:%d 数据:", GET_ID(id), IS_EFF(id)?"扩展帧":"标准帧"
                            , IS_RTR(id)?"远程帧":"数据帧", can.frame.can_dlc);
                        for (UINT i = 0; i < can.frame.can_dlc; ++i)
                        {
                            size_t item_len = strlen(item);
                            sprintf_s(&item[item_len], TMP_BUFFER_LEN-item_len, "%02X ", can.frame.data[i]);
                        }
                        qDebug() << QString(item);
                        responseHandle((unsigned char *)can.frame.data, can.frame.can_dlc);
                    }
                }
            }
        }
        if ((len = ZCAN_GetReceiveNum(chHandle, TYPE_CANFD))) {
            len = ZCAN_ReceiveFD(chHandle, canfd_data, 100, 50);
            if (channelActive) {
                char item[TMP_BUFFER_LEN] = {0};

                for (UINT i = 0; i < len; ++i) {
                    const ZCAN_ReceiveFD_Data& canfd = canfd_data[i];
                    const canid_t& id = canfd.frame.can_id;
                    if (canfd.frame.can_id == requestHandler.responseId) {
                        sprintf_s(item, "接收到CANFD ID:%08X %s %s 长度:%d 数据:", GET_ID(id), IS_EFF(id)?"扩展帧":"标准帧"
                            , IS_RTR(id)?"远程帧":"数据帧", canfd.frame.len);
                        for (UINT i = 0; i < canfd.frame.len; ++i)
                        {
                            size_t item_len = strlen(item);
                            sprintf_s(&item[item_len], TMP_BUFFER_LEN-item_len, "%02X ", canfd.frame.data[i]);
                        }
                        qDebug() << QString(item);
                        responseHandle((unsigned char *)canfd.frame.data, canfd.frame.len);
                    }
                }
            }
        }

        if (requestHandler.firstFram.size() > 0) {
            ZCAN_Transmit_Data can_data;
            memset(&can_data, 0, sizeof(can_data));
            can_data.frame.can_id = requestHandler.requestId;
            can_data.frame.can_dlc = requestHandler.firstFram.size();
            can_data.transmit_type = 0;

            memcpy_s(can_data.frame.data, sizeof(can_data.frame.data),\
                     requestHandler.firstFram.data(), requestHandler.firstFram.size());
            qDebug() <<  "firstFram ZCAN_Transmit" << ZCAN_Transmit(chHandle, &can_data, 1);
            requestHandler.firstFram.clear();
        }
    }
#endif /* __HAVE_CAN_ZLG__ */
}

bool CAN::sendRequest(const char *buf, quint32 len, quint32 sa, quint32 ta, bool supression, quint16 timeout)
{
#ifdef __HAVE_CAN_ZLG__
    if (channelActive) {
        return false;
    }
    QByteArray frame;
    char padd[8] = {0};

    canchannelComBox->setEnabled(false);
    chHandle = channelMap[canchannelComBox->currentText()];
    channelActive = true;
    requestHandler.requestId = ta;
    requestHandler.responseId = sa;
    requestHandler.supression = supression;
    requestHandler.timeout = timeout;
    requestHandler.frams.clear();
    requestHandler.firstFram.clear();

    responseHandler.responseFram.clear();
    responseHandler.expectLen = 0;

    this->activeSid = buf[0];
    if (len >= 8) {
        quint8 firstFrame0byte = 0;
        quint8 firstFrame1byte = len % 0xff;
        int offset = 0;
        int framenum = 0;
        int num = 0x01;

        firstFrame0byte |= 0x10;
        firstFrame0byte |= (len / 0xff) & 0x0f;

        frame.clear();
        frame.append(firstFrame0byte);
        frame.append(firstFrame1byte);
        frame.append(buf, 6);
        requestHandler.firstFram = frame;
        offset = 6;
        framenum = ((len - 6) / 7) + (((len - 6) % 7) > 0 ? 1: 0);
        qDebug() << "frame:" << frame.toHex(' ');
        for (int nn = 0; nn < framenum; nn++) {
            frame.clear();
            firstFrame0byte = 0;
            firstFrame0byte |= 0x20;
            firstFrame0byte |= num;
            frame.append(firstFrame0byte);
            frame.append(buf + 6 + (7*nn), len - (6 + (7*nn)) > 7 ? 7: len - (6 + (7*nn)));
            if (frame.size() < 8) {
                frame.append(padd, 8 - frame.size());
            }
            requestHandler.frams.enqueue(frame);

            qDebug() << "frame:" << frame.toHex(' ');
            num++;
            num = num > 0x0f ? 0x00 : num;
        }
    }
    else {
        frame.append(len);
        frame.append(buf, len);
        if (frame.size() < 8) {
            frame.append(padd, 8 - frame.size());
        }
        qDebug() << "short frame:" << frame.toHex(' ');
        requestHandler.firstFram = frame;
    }
#endif /* __HAVE_CAN_ZLG__ */
    return true;
}

bool CAN::sendFrames(int frames, quint16 delay)
{
#ifdef __HAVE_CAN_ZLG__
    if (requestHandler.frams.isEmpty()) {
        return false;
    }

    int count = requestHandler.frams.size() > frames ? frames : requestHandler.frams.size();

    ZCAN_Transmit_Data* pData = new ZCAN_Transmit_Data[count];
    for (int nn = 0; nn < count; nn++) {
        ZCAN_Transmit_Data can_data;
        QByteArray frame = requestHandler.frams.dequeue();

        memset(&can_data, 0, sizeof(can_data));
        can_data.frame.can_id = requestHandler.requestId;
        can_data.frame.can_dlc = frame.size();
        can_data.transmit_type = 0;

        memcpy_s(can_data.frame.data, sizeof(can_data.frame.data),\
                 frame.data(), frame.size());

        if (delay != 0) {
            can_data.frame.__pad |= TX_DELAY_SEND_FLAG;
            can_data.frame.__res0 = delay & 0xff;
            can_data.frame.__res1 = (delay >> 8) & 0xff;
        }

        memcpy_s(&pData[nn], sizeof(ZCAN_Transmit_Data), &can_data, sizeof(can_data));
    }

    qDebug() <<  "frams ZCAN_Transmit >> " << ZCAN_Transmit(chHandle, pData, count);
    delete[] pData;
#endif /* __HAVE_CAN_ZLG__ */
    return true;
}

bool CAN::responseHandle(unsigned char *buf, quint32 len)
{
#ifdef __HAVE_CAN_ZLG__
    if ((buf[0] & 0xf0) == 0x30) {
        sendFrames(buf[1] == 0 ? requestHandler.frams.size() : buf[1], buf[2]);
    }
    else if ((buf[0] & 0xf0) == 0x10) {
        ZCAN_Transmit_Data can_data;
        memset(&can_data, 0, sizeof(can_data));
        can_data.frame.can_id = requestHandler.requestId;
        can_data.frame.can_dlc = 8;
        can_data.transmit_type = 0;
        can_data.frame.data[0] = 0x30;
        qDebug() <<  "0x10 ZCAN_Transmit" << ZCAN_Transmit(chHandle, &can_data, 1);
        responseHandler.expectLen = ((buf[0] & 0x0f) << 8 )+ buf[1];
        responseHandler.responseFram.append((const char *)(buf + 2), 6);
    }
    else if ((buf[0] & 0xf0) == 0x20) {
        qDebug() << "(buf[0] & 0xf0) == 0x20";
        responseHandler.responseFram.append((const char *)(buf + 1), 7);
        if (responseHandler.responseFram.size() >= responseHandler.expectLen) {
            channelActive= false;
            qDebug() << "response finish:" << responseHandler.responseFram.toHex(' ');
            responseHandler.responseFram.resize(responseHandler.expectLen);
            qDebug() << "response finish:" << responseHandler.responseFram.toHex(' ');
            emit udsResponse(canchannelComBox->currentText(), normalFinish, responseHandler.responseFram);
            canchannelComBox->setEnabled(true);
        }
    }
    else {
        responseHandler.expectLen = buf[0] < len ? buf[0] : len - 1;
        responseHandler.responseFram.append((const char *)(buf + 1), responseHandler.expectLen);
        channelActive = false;
        qDebug() << "response finish:" << responseHandler.responseFram.toHex(' ');
        responseHandler.responseFram.resize(responseHandler.expectLen);
        qDebug() << "response finish:" << responseHandler.responseFram.toHex(' ');
        emit udsResponse(canchannelComBox->currentText(), normalFinish, responseHandler.responseFram);
        canchannelComBox->setEnabled(true);
    }
#endif /* __HAVE_CAN_ZLG__ */
    return true;
}

QString CAN::getCurrChannel()
{
    return canchannelComBox->currentText();
}
