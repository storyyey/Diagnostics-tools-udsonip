#include "doipclient.h"
#include "stream.h"

DoipClient::DoipClient(QObject *parent) : QObject(parent)
{
    doipClientConnInfoIsChange = false;
    udpSocket = nullptr;
    newDoipClientConn = nullptr;
    doipConnInfoModel = nullptr;

    sendStream.buff = new uint8_t[40960];
    sendStream.bufflen = 40960;
    sendStream.msglen = 0;

    recvStream.buff = new uint8_t[40960];
    recvStream.bufflen = 40960;
    recvStream.msglen = 0;

    doipConnScanTimer = new QTimer();
    doipConnScanTimer->setInterval(100);
    connect(doipConnScanTimer, &QTimer::timeout, this, [this]{
        if (doipConnInfoModel == nullptr) {
            return ;
        }
        for (int n = 0; n < doipClientConnectInfos.size(); n++) {
            DoipClientConnect *doipConn = doipClientConnectInfos.at(n);
            if (!doipConn->isRouteActive()) {
                if (doipConn->isReconnect()) {
                    qDebug() << "reconnect:" << doipConn;
                    if (doipConn->routeActiveRequest(doipConn->getServerHostAddress(), doipConn->getServerHostPort(), \
                                                 doipConn->getDoipVersion(), doipConn->getSourceAddress())) {
                        doipClientConnInfoIsChange = true;
                    }
                }
                else {
                    qDebug() << "delete:" << doipConn;
                    delete doipConn;
                    doipClientConnectInfos.removeAt(n);
                    doipClientConnInfoIsChange = true;
                    n--;
                }
            }
        }

        if (doipClientConnInfoIsChange) {
            doipConnInfoModel->removeRows(0, doipConnInfoModel->rowCount());
            DoipClientListBox->clear();
            doipClientConnInfoIsChange = false;
            for (int n = 0; n < doipClientConnectInfos.size(); n++) {
                DoipClientConnect *doipConn = doipClientConnectInfos.at(n);
                QList<QStandardItem *> itemList;

                QStandardItem *addrItem = new QStandardItem(doipConn->getServerHostAddress().toString());
                itemList.append(addrItem);

                QStandardItem *testerAddrItem = new QStandardItem(QString::number(doipConn->getSourceAddress(), 16));
                testerAddrItem->setToolTip(testerAddrItem->text());
                itemList.append(testerAddrItem);

                QStandardItem *logcAddrItem = new QStandardItem(QString::number(doipConn->getLogicTargetAddress(), 16));
                logcAddrItem->setToolTip(logcAddrItem->text());
                itemList.append(logcAddrItem);

                QStandardItem *vinItem = new QStandardItem(doipConn->getServerVin());
                itemList.append(vinItem);

                QStandardItem *upItem = new QStandardItem("0 b/s");
                itemList.append(upItem);

                QStandardItem *downItem = new QStandardItem("0 b/s");
                itemList.append(downItem);

                doipConnInfoModel->appendRow(itemList);
                DoipClientListBox->addItem(doipConn->getName());
                QPushButton *disconnectButton = new QPushButton("断开");
                disconnectButton->setIcon(QIcon(":/icon/disconnect.png"));
                if (doipConn->isReconnect() && !doipConn->isRouteActive()) {
                    disconnectButton->setText("重连中");
                }
                doipConnInfoView->setIndexWidget(doipConnInfoModel->index(n, 6) , disconnectButton);
                connect(disconnectButton, &QPushButton::clicked, this, [doipConn, disconnectButton] {
                    if (doipConn && doipConn->isRouteActive()) {
                        doipConn->disconnect();
                        disconnectButton->setText("连接");
                    }
                });
                QCheckBox *reconnectCheck = new QCheckBox();
                doipConnInfoView->setIndexWidget(doipConnInfoModel->index(n, 7) , reconnectCheck);
                if (doipConn->isReconnect()) {
                    reconnectCheck->setCheckState(Qt::Checked);
                }
                connect(reconnectCheck, &QCheckBox::clicked, this, [doipConn, reconnectCheck] {
                    if (reconnectCheck->checkState() == Qt::Checked) {
                        doipConn->setReconnect(true);
                    }
                    else {
                        doipConn->setReconnect(false);
                    }
                });
            }
        }
    });
    speedTimer = new QTimer(this);
    speedTimer->setInterval(1000);
    speedTimer->start();
    connect(speedTimer, &QTimer::timeout, this, [this] {
        for (int inedx = 0; inedx < doipClientConnectInfos.size(); inedx++) {
            DoipClientConnect *doipConn = doipClientConnectInfos.at(inedx);
            if (!doipConn) continue;
            QStandardItem *downItem = doipConnInfoModel->item(inedx, 5);
            if (downItem) {
                if (doipConn->downAverageByte / 1024 > 1) {
                    downItem->setText(QString("%1 K/s").arg(doipConn->downAverageByte / 1024));
                }
                else {
                    downItem->setText(QString("%1 B/s").arg(doipConn->downAverageByte));
                }
            }
            QStandardItem *upItem = doipConnInfoModel->item(inedx, 4);
            if (upItem) {
                if (doipConn->upAverageByte / 1024 > 1) {
                    upItem->setText(QString("%1 K/s").arg(doipConn->upAverageByte / 1024));
                }
                else {
                    upItem->setText(QString("%1 B/s").arg(doipConn->upAverageByte));
                }
            }
        }
    });
}

QWidget *DoipClient::DoipWidget()
{
    QRegExp regExp("((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})(\\.((2(5[0-5]|[0-4]\\d))|[0-1]?\\d{1,2})){3}");
    QValidator *ipvalidator = new QRegExpValidator(regExp, this);

    QRegExp hex2ByteregExp("^[0-9a-fA-F]{4}$");
    QValidator *hex2Bytevalidator = new QRegExpValidator(hex2ByteregExp, this);

    QWidget *mainWidget = new QWidget();
    mainWidget->setStyleSheet(btnCommonStyle);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainWidget->setLayout(mainLayout);

    QGroupBox *doipConfigWidget = new QGroupBox();
    doipConfigWidget->setTitle("DOIP基础参数设置");
    mainLayout->addWidget(doipConfigWidget);
    QGridLayout *configLayout = new QGridLayout();
    doipConfigWidget->setLayout(configLayout);

    QStringList doipProtocolTitle = {"TU", "ver", "~ver", "type", "len", "payload", "send"};
    QStandardItemModel *doipProtocolModel = new QStandardItemModel();

    doipProtocolModel->setColumnCount(doipProtocolTitle.length());
    doipProtocolModel->setHorizontalHeaderLabels(doipProtocolTitle);
    QTableView *doipProtocolView = new QTableView();
    doipProtocolView->verticalHeader()->hide();
    doipProtocolView->setSelectionMode(QAbstractItemView::NoSelection);
    //doipProtocolView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    doipProtocolView->setFixedHeight(55);
    doipProtocolView->horizontalHeader()->setFixedHeight(25);
    configLayout->addWidget(doipProtocolView, 0, 0, 1, 4);
    doipProtocolView->setModel(doipProtocolModel);

    doipProtocolView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    doipProtocolView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    doipProtocolView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    doipProtocolView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    doipProtocolView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    doipProtocolView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    doipProtocolView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);

    QList<QStandardItem *> doipProtocolItemList;
    doipProtocolItemList.append(new QStandardItem(""));
    QStandardItem *doipProtocolVerItem = new QStandardItem("02");
    // doipProtocolVerItem->setBackground(QBrush(((10 + 155) << 8) + 255));
    doipProtocolItemList.append(doipProtocolVerItem);
    QStandardItem *ndoipProtocolVerItem = new QStandardItem("fd");
    // ndoipProtocolVerItem->setBackground(QBrush(((20 + 155) << 8) + 255));
    doipProtocolItemList.append(ndoipProtocolVerItem);
    QStandardItem *doipProtocolTypeItem = new QStandardItem("00 01");
    // doipProtocolTypeItem->setBackground(QBrush(((30 + 155) << 8) + 255));
    doipProtocolItemList.append(doipProtocolTypeItem);
    QStandardItem *doipProtocolMsgLenItem = new QStandardItem("00 00 00 00");
    // doipProtocolMsgLenItem->setBackground(QBrush(((40 + 155) << 8) + 255));
    doipProtocolItemList.append(doipProtocolMsgLenItem);
    QStandardItem *doipPayloadItem = new QStandardItem("12 34");
    // doipPayloadItem->setBackground(QBrush(((50 + 155) << 8) + 255));
    doipProtocolItemList.append(doipPayloadItem);
    doipProtocolModel->appendRow(doipProtocolItemList);

    QComboBox* tcpudpSelect = new QComboBox();
    tcpudpSelect->addItems(QStringList() << "UDP" << "TCP");
    doipProtocolView->setIndexWidget(doipProtocolModel->index(0, 0) , tcpudpSelect);

    sendDoipBtn = new QPushButton("发送");
    sendDoipBtn->setIcon(QIcon(":/icon/send.png"));
    sendDoipBtn->setEnabled(false);
    doipProtocolView->setIndexWidget(doipProtocolModel->index(0, doipProtocolModel->columnCount() - 1) , sendDoipBtn);

    QPlainTextEdit *doipPayloadTextEdit = new QPlainTextEdit();
    doipProtocolView->setIndexWidget(doipProtocolModel->index(0, doipProtocolModel->columnCount() - 2) , doipPayloadTextEdit);
   // configLayout->addWidget(doipPayloadTextEdit, 1, 0, 1, 4);
    doipPayloadTextEdit->setMaximumHeight(40);
    connect(doipPayloadTextEdit, &QPlainTextEdit::textChanged, this, [doipProtocolMsgLenItem, doipPayloadTextEdit]{
        QString editText = doipPayloadTextEdit->toPlainText();
        QString regStr(editText.remove(QRegExp("[^0-9a-fA-F^ ]")));
        if (regStr.length() != doipPayloadTextEdit->toPlainText().length()) {
            if (regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8().length() % 2 == 0) {
                QByteArray msg = QByteArray::fromHex(regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                doipProtocolMsgLenItem->setText(QString().asprintf("%02x %02x %02x %02x", (msg.size() >> 24) & 0xff, (msg.size() >> 16) & 0xff, (msg.size() >> 8) & 0xff, msg.size() & 0xff));
                doipPayloadTextEdit->setPlainText(QString(msg.toHex(' ')));
            }
            else {
                doipPayloadTextEdit->setPlainText(editText);
            }
            doipPayloadTextEdit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
        }
        else {
            if (regStr.contains(QRegExp("[0-9a-f]{4}"))) {
                if (regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8().length() % 2 == 0) {
                    QByteArray msg = QByteArray::fromHex(regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                    doipProtocolMsgLenItem->setText(QString().asprintf("%02x %02x %02x %02x", (msg.size() >> 24) & 0xff, (msg.size() >> 16) & 0xff, (msg.size() >> 8) & 0xff, msg.size() & 0xff));
                    doipPayloadTextEdit->setPlainText(QString(msg.toHex(' ')));
                    doipPayloadTextEdit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
                }
            }
        }
    });
    connect(sendDoipBtn, &QPushButton::clicked, this, [this, doipPayloadTextEdit,\
            doipProtocolVerItem, doipProtocolTypeItem, doipProtocolMsgLenItem]{
        DoipHeader_t header;
        doipMessage_t msgInfo;
        const char *payload = nullptr;
        QByteArray msg;
        QString hexstr;

        hexstr = doipProtocolVerItem->text().remove(QRegExp("[^0-9a-fA-F]"));
        header.version = hexstr.toUInt(0, 16);
        header.inverse_version = ~header.version;
        hexstr = doipProtocolTypeItem->text().remove(QRegExp("[^0-9a-fA-F]"));
        header.payload_type = hexstr.toUInt(0, 16);
        hexstr = doipProtocolMsgLenItem->text().remove(QRegExp("[^0-9a-fA-F]"));
        header.payload_length = hexstr.toUInt(0, 16);
        QString editText = doipPayloadTextEdit->toPlainText();
        if (editText.size() > 0) {
            msg = QByteArray::fromHex(editText.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
            payload = msg.data();
            header.payload_length = msg.size();
        }
        sendDoipMessage(udpSocket, \
                        QHostAddress(srvAddrComBox->lineEdit()->text()),\
                        srvPortComBox->lineEdit()->text().toUInt(), header, payload, header.payload_length);
    });

    configLayout->addWidget(new QLabel("DOIP版本:"), 3, 0);
    doipVerComBox = new QComboBox();
    configLayout->addWidget(doipVerComBox, 3, 1);
    doipVerComBox->setEditable(true);
    doipVerComBox->lineEdit()->setText("2");
    doipVerComBox->lineEdit()->setValidator(new QRegExpValidator(QRegExp("^[0-9a-fA-F]{2}$"), this));
    doipVerComBox->addItem("2");
    connect(doipVerComBox->lineEdit(), &QLineEdit::textChanged, this, \
            [doipProtocolVerItem, ndoipProtocolVerItem](const QString &text){
            doipProtocolVerItem->setText(text);
            ndoipProtocolVerItem->setText(QByteArray(1, ~(text.toUInt(nullptr, 16))).toHex());
    });

    configLayout->addWidget(new QLabel("服务端IP:"), 4, 0);
    srvAddrComBox = new QComboBox();
    configLayout->addWidget(srvAddrComBox, 4, 1);
    srvAddrComBox->setEditable(true);
    srvAddrComBox->lineEdit()->setValidator(ipvalidator);
    srvAddrComBox->lineEdit()->setText("192.168.225.1");
    srvAddrComBox->addItem("192.168.225.1");

    configLayout->addWidget(new QLabel("服务端Port:"), 4, 2);
    srvPortComBox = new QComboBox();
    configLayout->addWidget(srvPortComBox, 4, 3);
    srvPortComBox->setEditable(true);
    srvPortComBox->lineEdit()->setValidator(new QRegExpValidator(QRegExp("^[0-9]{10}$"), this));
    srvPortComBox->lineEdit()->setText("13400");
    srvPortComBox->addItem("13400");

    configLayout->addWidget(new QLabel("VIN:"), 5, 0);
    vinComBox = new QComboBox();
    configLayout->addWidget(vinComBox, 5, 1);
    vinComBox->setEditable(true);
    vinComBox->lineEdit()->setValidator(new QRegExpValidator(QRegExp("^[a-zA-Z0-9]{17}$"), this));

    configLayout->addWidget(new QLabel("EID:"), 5, 2);
    EIDComBox = new QComboBox();
    configLayout->addWidget(EIDComBox, 5, 3);
    EIDComBox->setEditable(true);
    EIDComBox->lineEdit()->setValidator(new QRegExpValidator(QRegExp("^[a-zA-Z0-9]{17}$"), this));

    configLayout->addWidget(new QLabel("服务端物理地址(HEX):"), 6, 0);
    phyAddrComBox = new QComboBox();
    configLayout->addWidget(phyAddrComBox, 6, 1);
    phyAddrComBox->setEditable(true);
    phyAddrComBox->lineEdit()->setValidator(hex2Bytevalidator);
    connect(phyAddrComBox->lineEdit(), &QLineEdit::textChanged, this, \
            [](const QString &text){

    });

    configLayout->addWidget(new QLabel("服务端功能地址(HEX):"), 6, 2);
    funcAddrComBox = new QComboBox();
    configLayout->addWidget(funcAddrComBox, 6, 3);
    funcAddrComBox->setEditable(true);
    funcAddrComBox->lineEdit()->setValidator(hex2Bytevalidator);

    configLayout->addWidget(new QLabel("客户端源地址(HEX):"), 7, 0);
    saAddrComBox = new QComboBox();
    configLayout->addWidget(saAddrComBox, 7, 1);
    saAddrComBox->setEditable(true);
    saAddrComBox->lineEdit()->setValidator(hex2Bytevalidator);
    connect(saAddrComBox->lineEdit(), &QLineEdit::textChanged, this, \
            [](const QString &text){

    });

    configLayout->addWidget(new QLabel("客户端IP:"), 7, 2);
    clientAddrComBox = new QComboBox();
    configLayout->addWidget(clientAddrComBox, 7, 3);
    QList<QHostAddress> allAddrList = QNetworkInterface::allAddresses();
    for (int i = 0; i < allAddrList.count(); i++){
        QHostAddress var = allAddrList[i];
        if (var.protocol() == QAbstractSocket::IPv4Protocol) {
            clientAddrComBox->addItem(var.toString());
        }
    }

    configLayout->addWidget(new QLabel("客户端Port:"), 8, 0);
    clientPortComBox = new QComboBox();
    configLayout->addWidget(clientPortComBox, 8, 1);
    clientPortComBox->setEditable(true);
    clientPortComBox->lineEdit()->setValidator(new QRegExpValidator(QRegExp("^[0-9]{10}$"), this));
    clientPortComBox->lineEdit()->setText("13400");
    clientPortComBox->addItem("ANY");

    configLayout->addWidget(new QLabel("接收缓冲区大小:"), 9, 0);
    recvBuffLineEdit = new QLineEdit();
    configLayout->addWidget(recvBuffLineEdit, 9, 1);
    recvBuffLineEdit->setValidator(new QIntValidator(0, 400000000, recvBuffLineEdit));
    recvBuffLineEdit->setText("40960");

    configLayout->addWidget(new QLabel("发送缓冲区大小:"), 9, 2);
    sendBuffLineEdit = new QLineEdit();
    configLayout->addWidget(sendBuffLineEdit, 9, 3);
    sendBuffLineEdit->setValidator(new QIntValidator(0, 400000000, sendBuffLineEdit));
    sendBuffLineEdit->setText("40960");

    createDoipClient = new QPushButton("创建DOIP客户端");
    createDoipClient->setIcon(QIcon(":/icon/create.png"));
    configLayout->addWidget(createDoipClient, 11, 0, 1, 2);
    connect(createDoipClient, &QPushButton::clicked, this, [this] {
        if (udpSocket == nullptr) {
            udpSocket = new QUdpSocket();
            connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
            if (clientPortComBox->lineEdit()->text() == "ANY") {
                udpSocket->bind(QHostAddress(clientAddrComBox->currentText()));
            }
            else {
                udpSocket->bind(QHostAddress(clientAddrComBox->currentText()), \
                                             clientPortComBox->lineEdit()->text().toUInt());
            }
            vehicleIdReqSendBtn->setEnabled(true);
            //vehicleActiveSendBtn->setEnabled(true);
            newConBtn->setEnabled(true);
            sendDoipBtn->setEnabled(true);
            destroyDoipClient->setEnabled(true);
            createDoipClient->setEnabled(false);
            writeConfigHistoryCache();
        }
    });

    destroyDoipClient = new QPushButton("关闭DOIP客户端");
    destroyDoipClient->setIcon(QIcon(":/icon/close.png"));
    destroyDoipClient->setEnabled(false);
    configLayout->addWidget(destroyDoipClient, 11, 2, 1, 2);
    connect(destroyDoipClient, &QPushButton::clicked, this, [this] {
        if (udpSocket != nullptr) {
            delete udpSocket ;
            udpSocket = nullptr;
            vehicleIdReqSendBtn->setEnabled(false);
            //vehicleActiveSendBtn->setEnabled(false);
            newConBtn->setEnabled(false);
            sendDoipBtn->setEnabled(false);
            destroyDoipClient->setEnabled(false);
            createDoipClient->setEnabled(true);
        }
    });
/*------------------------车辆信息请求 QGroupBox start----------------------------------*/
    QGroupBox *newConWidget = new QGroupBox();
    newConWidget->setTitle("连接激活DOIP服务端");
    QVBoxLayout *newConMainLayout = new QVBoxLayout();

    QHBoxLayout *newConLayout = new QHBoxLayout();
    newConMainLayout->addLayout(newConLayout);
    newConWidget->setLayout(newConMainLayout);

    newConLayout->addWidget(new QLabel("超时时间(ms)"));
    doipConnTimeoutEdit = new QLineEdit();
    doipConnTimeoutEdit->setMaximumWidth(60);
    doipConnTimeoutEdit->setValidator(new QRegExpValidator(QRegExp("^[0-9]{6}$"), this));
    doipConnTimeoutEdit->setText("2000");
    newConLayout->addWidget(doipConnTimeoutEdit);
    newConLayout->addStretch();

    doipConnResponseLabel = new QLabel("连接时间");
    newConLayout->addWidget(doipConnResponseLabel);
    doipConnRespTimer = new QTimer();
    connect(doipConnRespTimer, &QTimer::timeout, this, [this]{
        newConBtn->setEnabled(true);
        doipConnResponseLabel->setText("<font color = red><u>连接超时</u></font>");
        doipConnRespTimer->stop();
        if (newDoipClientConn) {
            delete newDoipClientConn;
            newDoipClientConn = nullptr;
        }
    });

    newConBtn = new QPushButton("连接");
    newConBtn->setIcon(QIcon(":/icon/connect.png"));
    newConBtn->setEnabled(false);
    newConLayout->addWidget(newConBtn);
    connect(newConBtn, &QPushButton::clicked, this, [this] {
        if (newDoipClientConn != nullptr) {
            return ;
        }
        vehicleIdReqSendBtn->click();

        quint32 rl = recvBuffLineEdit->text().toUInt();
        quint32 sl = sendBuffLineEdit->text().toUInt();
        if (rl + sl > 100 * 1024 * 1024) {
            switch (QMessageBox::information(0, tr("缓冲区设置"), tr(QString("接收和发送缓冲区大小为%1字节，可能造成软件闪退是否继续设置？").arg(rl + sl).toStdString().c_str()), tr("是"), tr("否"), 0, 1 )) {
              case 0: break;
              case 1: default: return ; break;
            }
        }
        newDoipClientConn = new DoipClientConnect(rl, sl);
        connect(newDoipClientConn, SIGNAL(newDoipMessage(const doipMessage_t &)),\
                this, SLOT(addDoipMessage(const doipMessage_t &)));
        connect(newDoipClientConn, SIGNAL(routeActiveStateChange(DoipClientConnect *, bool)),\
                this, SLOT(RouteActiveState(DoipClientConnect *, bool)));
        connect(newDoipClientConn, SIGNAL(diagnosisFinish(DoipClientConnect *, DoipClientConnect::sendDiagFinishState, QByteArray &)),\
                this, SLOT(diagnosisResponse(DoipClientConnect *, DoipClientConnect::sendDiagFinishState, QByteArray &)));
        newDoipClientConn->routeActiveRequest(QHostAddress(srvAddrComBox->lineEdit()->text()),\
                                              srvPortComBox->lineEdit()->text().toUInt(),\
                                              doipVerComBox->lineEdit()->text().toUInt(), \
                                              saAddrComBox->lineEdit()->text().toInt(nullptr, 16), \
                                                 doipConnTimeoutEdit->text().toInt());
        doipConnResponseLabel->setText("等待连接");
        doipConnReqTime = QTime::currentTime();
        doipConnRespTimer->setInterval(doipConnTimeoutEdit->text().toInt());
        doipConnRespTimer->start();
        newConBtn->setEnabled(false);
    });

    QStringList doipConnInfoTitle = {"IP", "SA", "TA", "VIN", "上行", "下行", "操作", "重连"};
    doipConnInfoModel = new QStandardItemModel();
    doipConnInfoModel->setColumnCount(doipConnInfoTitle.length());
    doipConnInfoModel->setHorizontalHeaderLabels(doipConnInfoTitle);
    doipConnInfoView = new QTableView();
    newConMainLayout->addWidget(doipConnInfoView);
    newConMainLayout->addStretch() ;
    doipConnInfoView->setModel(doipConnInfoModel);
    doipConnInfoView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // doipConnInfoView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    // doipConnInfoView->setColumnWidth(0, 100);
    // doipConnInfoView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    // doipConnInfoView->setColumnWidth(1, 80);
    // doipConnInfoView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    // doipConnInfoView->setColumnWidth(2, 80);
    // doipConnInfoView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    // doipConnInfoView->setColumnWidth(4, 80);
    // doipConnInfoView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    // doipConnInfoView->setColumnWidth(5, 80);
    // doipConnInfoView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    // doipConnInfoView->setColumnWidth(6, 80);
    // doipConnInfoView->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
    // doipConnInfoView->setColumnWidth(7, 30);
    doipConnInfoView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    doipConnInfoView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    doipConnInfoView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    doipConnInfoView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    doipConnInfoView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    doipConnInfoView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    doipConnInfoView->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    mainLayout->addWidget(newConWidget);
/*------------------------车辆信息请求 QGroupBox end----------------------------------*/

/*------------------------车辆信息请求 QGroupBox start----------------------------------*/
    QGroupBox *vehicleIdReqWidget = new QGroupBox();
    vehicleIdReqWidget->setTitle("车辆信息请求");
    QVBoxLayout *vehicleIdMainLayout = new QVBoxLayout();

    QHBoxLayout *vehicleIdLayout = new QHBoxLayout();
    vehicleIdMainLayout->addLayout(vehicleIdLayout);
    vehicleIdReqWidget->setLayout(vehicleIdMainLayout);

    QCheckBox *vehicleIdReqVINCheckBox = new QCheckBox("包含VIN");
    vehicleIdLayout->addWidget(vehicleIdReqVINCheckBox);
    //vehicleIdReqVINCheckBox->setCheckState(Qt::Checked);
    QLabel *vehicleIdReqVINLabel = new QLabel();
    vehicleIdLayout->addWidget(vehicleIdReqVINLabel);
    connect(vehicleIdReqVINCheckBox, &QCheckBox::stateChanged, this, [this, vehicleIdReqVINLabel](int state) {
        if (state != Qt::Checked) {
            vehicleIdReqVINLabel->hide();
        }
        else {
            vehicleIdReqVINLabel->setText(vinComBox->lineEdit()->text());
            vehicleIdReqVINLabel->show();
        }
    });
    vehicleIdLayout->addStretch();

    QCheckBox *vehicleIdReqEIDCheckBox = new QCheckBox("包含EID");
    //vehicleIdReqEIDCheckBox->setCheckState(Qt::Checked);
    vehicleIdLayout->addWidget(vehicleIdReqEIDCheckBox);
    QLabel *vehicleIdReqEIDLabel = new QLabel();
    vehicleIdLayout->addWidget(vehicleIdReqEIDLabel);
    connect(vehicleIdReqEIDCheckBox, &QCheckBox::stateChanged, this, [this, vehicleIdReqEIDLabel](int state) {
        if (state != Qt::Checked) {
            vehicleIdReqEIDLabel->hide();
        }
        else {
            vehicleIdReqEIDLabel->setText(EIDComBox->lineEdit()->text());
            vehicleIdReqEIDLabel->show();        }
    });
    vehicleIdLayout->addStretch();

    vehicleIdLayout->addWidget(new QLabel("超时时间(ms)"));
    vehicleIdTimeoutEdit = new QLineEdit();
    vehicleIdTimeoutEdit->setMaximumWidth(40);
    vehicleIdTimeoutEdit->setValidator(new QRegExpValidator(QRegExp("^[0-9]{6}$"), this));
    vehicleIdTimeoutEdit->setText("2000");
    vehicleIdLayout->addWidget(vehicleIdTimeoutEdit);
    vehicleIdResponseLabel = new QLabel("响应时间");
    vehicleIdLayout->addWidget(vehicleIdResponseLabel);
    vehicleIdRespTimer = new QTimer();
    connect(vehicleIdRespTimer, &QTimer::timeout, this, [this]{
        vehicleIdReqSendBtn->setEnabled(true);
        vehicleIdResponseLabel->setText("<font color = red><u>响应超时</u></font>");
        vehicleIdRespTimer->stop();
    });

    vehicleIdReqSendBtn = new QPushButton("发送");
    vehicleIdReqSendBtn->setIcon(QIcon(":/icon/send.png"));
    vehicleIdReqSendBtn->setEnabled(false);
    vehicleIdLayout->addWidget(vehicleIdReqSendBtn);

    connect(vehicleIdReqSendBtn, &QPushButton::clicked, this, [this,\
            vehicleIdReqVINCheckBox, vehicleIdReqEIDCheckBox] {
        DoipHeader_t header;
        doipMessage_t msgInfo;
        const char *payload = nullptr;

        header.version = doipVerComBox->lineEdit()->text().toUInt();
        header.inverse_version = ~header.version;
        header.payload_type = vehicleIdentificationRequest;
        header.payload_length = 0;
        if (vehicleIdReqVINCheckBox->checkState() == Qt::Checked) {
            payload = vinComBox->currentText().toStdString().c_str();
            header.payload_length = vinComBox->currentText().length();
            header.payload_type = vehicleIdentificationVINRequest;
        }
        else if (vehicleIdReqEIDCheckBox->checkState() == Qt::Checked) {
            payload = EIDComBox->currentText().toStdString().c_str();
            header.payload_length = EIDComBox->currentText().length();
            header.payload_type = vehicleIdentificationEIDRequest;
        }
        sendDoipMessage(udpSocket, \
                        QHostAddress(srvAddrComBox->lineEdit()->text()),\
                        srvPortComBox->lineEdit()->text().toUInt(), header, payload, header.payload_length);
        vehicleIdReqTime = QTime::currentTime();
        vehicleIdReqSendBtn->setEnabled(false);
        vehicleIdResponseLabel->setText("等待响应");
        vehicleIdRespTimer->setInterval(vehicleIdTimeoutEdit->text().toInt());
        vehicleIdRespTimer->start();
    });

    QStringList vehicleInfoTitle = {"IP地址", "地址", "VIN", "EID", "GID"};
    vehicleInfoModel = new QStandardItemModel();
    vehicleInfoModel->setColumnCount(vehicleInfoTitle.length());
    vehicleInfoModel->setHorizontalHeaderLabels(vehicleInfoTitle);
    QTableView *vehicleInfoView = new QTableView();
    vehicleIdMainLayout->addWidget(vehicleInfoView);
    vehicleIdMainLayout->addStretch() ;
    vehicleInfoView->setModel(vehicleInfoModel);
    vehicleInfoView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    vehicleInfoView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    vehicleInfoView->setColumnWidth(0, 100);
    vehicleInfoView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    vehicleInfoView->setColumnWidth(1, 50);
    vehicleInfoView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    vehicleInfoView->setColumnWidth(2, 150);
    vehicleInfoView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vehicleInfoView->setAlternatingRowColors(true);
    vehicleInfoView->setContextMenuPolicy(Qt::CustomContextMenu);
    vehicleInfoView->setSortingEnabled(true);

    mainLayout->addWidget(vehicleIdReqWidget);
/*------------------------车辆信息请求 QGroupBox end----------------------------------*/

/*------------------------车辆激活请求 QGroupBox start----------------------------------*/
#if 0
    QGroupBox *vehicleActiveWidget = new QGroupBox();
    vehicleActiveWidget->setTitle("DOIP协议请求");
    QHBoxLayout *vehicleActiveLayout = new QHBoxLayout();
    vehicleActiveWidget->setLayout(vehicleActiveLayout);

    vehicleActiveSendBtn = new QPushButton("发送");
    vehicleActiveSendBtn->setEnabled(false);
    vehicleActiveLayout->addWidget(vehicleActiveSendBtn);
    connect(vehicleIdReqSendBtn, &QPushButton::clicked, this, [this] {

    });
    mainLayout->addWidget(vehicleActiveWidget);
#endif
/*------------------------车辆激活请求 QGroupBox end----------------------------------*/
/*------------------------诊断请求 QGroupBox start----------------------------------*/
    QGroupBox *diagWidget = new QGroupBox();
    diagWidget->setTitle("诊断请求");
    QVBoxLayout *diagMainLayout = new QVBoxLayout();

    QPlainTextEdit *diagReqMsgEdit = new QPlainTextEdit();
    diagReqMsgEdit->setMaximumHeight(35);
    diagMainLayout->addWidget(diagReqMsgEdit);

    QHBoxLayout *diagEditLayout = new QHBoxLayout();
    diagMainLayout->addLayout(diagEditLayout);

    diagEditLayout->addWidget(new QLabel("字符(hex):"));
    QLineEdit *diagRandCharEdit = new QLineEdit();
    diagRandCharEdit->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{2}$"), this));
    diagRandCharEdit->setMaximumWidth(30);
    diagEditLayout->addWidget(diagRandCharEdit);

    diagEditLayout->addWidget(new QLabel("长度(byte):"));
    QLineEdit *diagRandMsgLenEdit = new QLineEdit();
    diagRandMsgLenEdit->setValidator(new QRegExpValidator(QRegExp("^[0-9]{9}$"), this));
    diagRandMsgLenEdit->setMaximumWidth(50);
    diagEditLayout->addWidget(diagRandMsgLenEdit);

    QPushButton *diagRandMsgBtn = new QPushButton("生成");
    diagRandMsgBtn->setIcon(QIcon(":/icon/generate.png"));
    diagEditLayout->addWidget(diagRandMsgBtn);

    diagEditLayout->addStretch();
    QPushButton *diagCleanBtn = new QPushButton("清除");
    diagCleanBtn->setIcon(QIcon(":/icon/delete.png"));
    diagEditLayout->addWidget(diagCleanBtn);

    QLabel *diagMsgLen = new QLabel("0/byte");
    diagMsgLen->setFixedWidth(60);
    diagEditLayout->addWidget(diagMsgLen);

    connect(diagCleanBtn, &QPushButton::clicked, this, [diagReqMsgEdit, diagMsgLen] {
        diagReqMsgEdit->clear();
        diagMsgLen->setText("0/byte");
    });

    connect(diagRandMsgBtn, &QPushButton::clicked, this, [diagReqMsgEdit, diagRandMsgLenEdit, diagRandCharEdit] {
        QString randMsg;

        if (diagRandCharEdit->text().size() == 1) {
            diagRandCharEdit->setText("0" + diagRandCharEdit->text());
        }
        for (int n = 0; n < diagRandMsgLenEdit->text().toInt(); n++) {
            if (diagRandCharEdit->text().size() == 0) {
                randMsg.append(QString(QByteArray(1, qrand() % (0xff + 1)).toHex()));
            }
            else {
                randMsg.append(diagRandCharEdit->text());
            }
        }
        diagReqMsgEdit->setPlainText(randMsg);
    });

    connect(diagReqMsgEdit, &QPlainTextEdit::textChanged, this, [diagReqMsgEdit, diagMsgLen]{
        QString editText = diagReqMsgEdit->toPlainText();
        QString regStr(editText.remove(QRegExp("[^0-9a-fA-F^ ]")));
        if (regStr.length() != diagReqMsgEdit->toPlainText().length()) {
            if (regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8().length() % 2 == 0) {
                QByteArray msg = QByteArray::fromHex(regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                diagMsgLen->setText(QString("%1/byte").arg(msg.size()));
                diagReqMsgEdit->setPlainText(QString(msg.toHex(' ')));
            }
            else {
                diagReqMsgEdit->setPlainText(editText);
            }
            diagReqMsgEdit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
        }
        else {
            if (regStr.contains(QRegExp("[0-9a-f]{4}"))) {
                if (regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8().length() % 2 == 0) {
                    QByteArray msg = QByteArray::fromHex(regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                    diagMsgLen->setText(QString("%1/byte").arg(msg.size()));
                    diagReqMsgEdit->setPlainText(QString(msg.toHex(' ')));
                    diagReqMsgEdit->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
                }
            }
        }
    });

    QHBoxLayout *diagLayout = new QHBoxLayout();
    diagMainLayout->addLayout(diagLayout);
    diagWidget->setLayout(diagMainLayout);
    diagWidget->hide();

    diagLayout->addWidget(new QLabel("doip客户端:"));
    DoipClientListBox = new QComboBox();
    //DoipClientListBox->setMinimumWidth(200);
    diagLayout->addWidget(DoipClientListBox);

    diagLayout->addWidget(new QLabel("TA:"));
    QComboBox *diagTaEdit = new QComboBox();
    diagTaEdit->setEditable(true);
    diagTaEdit->setValidator(hex2Bytevalidator);
    diagTaEdit->setMaximumWidth(60);
    diagLayout->addWidget(diagTaEdit);
    connect(DoipClientListBox, &QComboBox::currentTextChanged, this, [this, diagTaEdit](const QString &text){
        qDebug() << DoipClientListBox->currentIndex();
        if (DoipClientListBox->currentIndex() >= 0 && \
            DoipClientListBox->currentIndex() < doipClientConnectInfos.size()) {
            DoipClientConnect *DoipConn = doipClientConnectInfos.at(DoipClientListBox->currentIndex());
            if (DoipConn && DoipConn->isRouteActive()) {
                diagTaEdit->addItem(QString::number(DoipConn->getLogicTargetAddress(), 16));
                diagTaEdit->addItem(QString::number(DoipConn->getFuncTargetAddress(), 16));
                diagTaEdit->lineEdit()->setText(QString::number(DoipConn->getLogicTargetAddress(), 16));
            }
        }
        else {
            diagTaEdit->clear();
        }
    });

    QCheckBox *diagSupressCheckBox = new QCheckBox("抑制响应");
    diagLayout->addWidget(diagSupressCheckBox);

    diagLayout->addWidget(new QLabel("超时时间(ms):"));
    QLineEdit *diagTimeoutEdit = new QLineEdit();
    diagTimeoutEdit->setText("2000");
    diagTimeoutEdit->setMaximumWidth(40);
    diagLayout->addWidget(diagTimeoutEdit);

    diagLayout->addStretch();
    diagSendBtn = new QPushButton("发送");
    diagSendBtn->setIcon(QIcon(":/icon/send.png"));
    diagLayout->addWidget(diagSendBtn);
    connect(diagSendBtn, &QPushButton::clicked, this, [this, diagReqMsgEdit, diagTaEdit, diagSupressCheckBox, diagTimeoutEdit] {
        int index = DoipClientListBox->currentIndex();
        if (index >= 0 && index < doipClientConnectInfos.size()) {
            udsDoipClientConn = doipClientConnectInfos.at(index);
            QByteArray sendMsg = QByteArray::fromHex(diagReqMsgEdit->toPlainText().remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
            udsDoipClientConn->diagnosisRequest(sendMsg, sendMsg.size(), diagTaEdit->lineEdit()->text().toInt(nullptr, 16),\
                                       (diagSupressCheckBox->checkState() ==  Qt::Checked) ? true : false, \
                                       diagTimeoutEdit->text().toUInt());
        }
    });

    diagRespMsgEdit = new QPlainTextEdit();
    diagRespMsgEdit->setMaximumHeight(25);
    diagMainLayout->addWidget(diagRespMsgEdit);

    mainLayout->addWidget(diagWidget);
/*------------------------诊断请求 QGroupBox end----------------------------------*/
    mainLayout->addStretch() ;

    backupCacheInit();

    return mainWidget;
}

void DoipClient::backupCacheInit()
{
    readConfigHistoryCache();

    QSettings settings(serviceBackCacheDir + "doipOperationSettings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("utf-8"));

    doipVerComBox->setCurrentText(settings.value("doipClient/doipVerComBox").toString());
    srvAddrComBox->setCurrentText(settings.value("doipClient/srvAddrComBox").toString());
    srvPortComBox->setCurrentText(settings.value("doipClient/srvPortComBox").toString());
    vinComBox->setCurrentText(settings.value("doipClient/vinComBox").toString());
    EIDComBox->setCurrentText(settings.value("doipClient/EIDComBox").toString());
    phyAddrComBox->setCurrentText(settings.value("doipClient/phyAddrComBox").toString());
    funcAddrComBox->setCurrentText(settings.value("doipClient/funcAddrComBox").toString());
    saAddrComBox->setCurrentText(settings.value("doipClient/saAddrComBox").toString());
    clientAddrComBox->setCurrentText(settings.value("doipClient/clientAddrComBox").toString());
    clientPortComBox->setCurrentText(settings.value("doipClient/clientPortComBox").toString());
    recvBuffLineEdit->setText(settings.value("doipClient/recvBuffLineEdit").toString());
    sendBuffLineEdit->setText(settings.value("doipClient/sendBuffLineEdit").toString());
}

void DoipClient::operationCache()
{
    QSettings settings(serviceBackCacheDir + "doipOperationSettings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("utf-8"));

    settings.setValue("doipClient/doipVerComBox", doipVerComBox->currentText());
    settings.setValue("doipClient/srvAddrComBox", srvAddrComBox->currentText());
    settings.setValue("doipClient/srvPortComBox", srvPortComBox->currentText());
    settings.setValue("doipClient/vinComBox", vinComBox->currentText());
    settings.setValue("doipClient/EIDComBox", EIDComBox->currentText());
    settings.setValue("doipClient/phyAddrComBox", phyAddrComBox->currentText());
    settings.setValue("doipClient/funcAddrComBox", funcAddrComBox->currentText());
    settings.setValue("doipClient/saAddrComBox", saAddrComBox->currentText());
    settings.setValue("doipClient/clientAddrComBox", clientAddrComBox->currentText());
    settings.setValue("doipClient/clientPortComBox", clientPortComBox->currentText());
    settings.setValue("doipClient/recvBuffLineEdit", recvBuffLineEdit->text());
    settings.setValue("doipClient/sendBuffLineEdit", sendBuffLineEdit->text());

    settings.sync();
}

void DoipClient::writeConfigHistoryCache()
{
    QStringList vstrs;
    QString stritem;
    QSettings settings(serviceBackCacheDir + "doipOperationSettings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("utf-8"));

    vstrs = settings.value("doipClient/doipVerComBoxHistory").toStringList();
    stritem = doipVerComBox->currentText();
    if (stritem.size() > 0 && !vstrs.contains(stritem)) {
        if (vstrs.size() > 9) vstrs.removeAt(0);
        vstrs.append(stritem);
        doipVerComBox->clear();
        doipVerComBox->addItems(vstrs);
        doipVerComBox->setCurrentText(stritem);
        settings.setValue("doipClient/doipVerComBoxHistory", vstrs);
    }
    vstrs = settings.value("doipClient/srvAddrComBoxHistory").toStringList();
    stritem = srvAddrComBox->currentText();
    if (stritem.size() > 0 && !vstrs.contains(stritem)) {
        if (vstrs.size() > 9) vstrs.removeAt(0);
        vstrs.append(stritem);
        srvAddrComBox->clear();
        srvAddrComBox->addItems(vstrs);
        srvAddrComBox->setCurrentText(stritem);
        settings.setValue("doipClient/srvAddrComBoxHistory", vstrs);
    }
    vstrs = settings.value("doipClient/srvPortComBoxHistory").toStringList();
    stritem = srvPortComBox->currentText();
    if (stritem.size() > 0 && !vstrs.contains(stritem)) {
        if (vstrs.size() > 9) vstrs.removeAt(0);
        vstrs.append(stritem);
        srvPortComBox->clear();
        srvPortComBox->addItems(vstrs);
        srvPortComBox->setCurrentText(stritem);
        settings.setValue("doipClient/srvPortComBoxHistory", vstrs);
    }
    vstrs = settings.value("doipClient/vinComBoxHistory").toStringList();
    stritem = vinComBox->currentText();
    if (stritem.size() > 0 && !vstrs.contains(stritem)) {
        if (vstrs.size() > 9) vstrs.removeAt(0);
        vstrs.append(stritem);
        vinComBox->clear();
        vinComBox->addItems(vstrs);
        vinComBox->setCurrentText(stritem);
        settings.setValue("doipClient/vinComBoxHistory", vstrs);
    }
    vstrs = settings.value("doipClient/EIDComBoxHistory").toStringList();
    stritem = EIDComBox->currentText();
    if (stritem.size() > 0 && !vstrs.contains(stritem)) {
        if (vstrs.size() > 9) vstrs.removeAt(0);
        vstrs.append(stritem);
        EIDComBox->clear();
        EIDComBox->addItems(vstrs);
        EIDComBox->setCurrentText(stritem);
        settings.setValue("doipClient/EIDComBoxHistory", vstrs);
    }
    vstrs = settings.value("doipClient/phyAddrComBoxHistory").toStringList();
    stritem = phyAddrComBox->currentText();
    if (stritem.size() > 0 && !vstrs.contains(stritem)) {
        if (vstrs.size() > 9) vstrs.removeAt(0);
        vstrs.append(stritem);
        phyAddrComBox->clear();
        phyAddrComBox->addItems(vstrs);
        phyAddrComBox->setCurrentText(stritem);
        settings.setValue("doipClient/phyAddrComBoxHistory", vstrs);
    }
    vstrs = settings.value("doipClient/funcAddrComBoxHistory").toStringList();
    stritem = funcAddrComBox->currentText();
    if (stritem.size() > 0 && !vstrs.contains(stritem)) {
        if (vstrs.size() > 9) vstrs.removeAt(0);
        vstrs.append(stritem);
        funcAddrComBox->clear();
        funcAddrComBox->addItems(vstrs);
        funcAddrComBox->setCurrentText(stritem);
        settings.setValue("doipClient/funcAddrComBoxHistory", vstrs);
    }
    vstrs = settings.value("doipClient/saAddrComBoxHistory").toStringList();
    stritem = saAddrComBox->currentText();
    if (stritem.size() > 0 && !vstrs.contains(stritem)) {
        if (vstrs.size() > 9) vstrs.removeAt(0);
        vstrs.append(stritem);
        saAddrComBox->clear();
        saAddrComBox->addItems(vstrs);
        saAddrComBox->setCurrentText(stritem);
        settings.setValue("doipClient/saAddrComBoxHistory", vstrs);
    }

    vstrs = settings.value("doipClient/clientPortComBoxHistory").toStringList();
    stritem = clientPortComBox->currentText();
    if (stritem.size() > 0 && !vstrs.contains(stritem)) {
        if (vstrs.size() > 9) vstrs.removeAt(0);
        clientPortComBox->clear();
        clientPortComBox->addItems(vstrs);
        clientPortComBox->setCurrentText(stritem);
        settings.setValue("doipClient/clientPortComBoxHistory", vstrs);
    }
}

void DoipClient::readConfigHistoryCache()
{
    QStringList vstrs;
    QSettings settings(serviceBackCacheDir + "doipOperationSettings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("utf-8"));

    vstrs = settings.value("doipClient/doipVerComBoxHistory").toStringList();
    doipVerComBox->clear();
    doipVerComBox->addItems(vstrs);

    vstrs = settings.value("doipClient/srvAddrComBoxHistory").toStringList();
    srvAddrComBox->clear();
    srvAddrComBox->addItems(vstrs);

    vstrs = settings.value("doipClient/srvPortComBoxHistory").toStringList();
    srvPortComBox->clear();
    srvPortComBox->addItems(vstrs);

    vstrs = settings.value("doipClient/vinComBoxHistory").toStringList();
    vinComBox->clear();
    vinComBox->addItems(vstrs);

    vstrs = settings.value("doipClient/EIDComBoxHistory").toStringList();
    EIDComBox->clear();
    EIDComBox->addItems(vstrs);

    vstrs = settings.value("doipClient/phyAddrComBoxHistory").toStringList();
    phyAddrComBox->clear();
    phyAddrComBox->addItems(vstrs);

    vstrs = settings.value("doipClient/funcAddrComBoxHistory").toStringList();
    funcAddrComBox->clear();
    funcAddrComBox->addItems(vstrs);

    vstrs = settings.value("doipClient/saAddrComBoxHistory").toStringList();
    saAddrComBox->clear();
    saAddrComBox->addItems(vstrs);

    vstrs = settings.value("doipClient/clientPortComBoxHistory").toStringList();
    clientPortComBox->clear();
    clientPortComBox->addItems(vstrs);
}

bool DoipClient::DoipHeaderEncode(frameStream_t &stream, DoipHeader_t &header)
{
    stream_t Sp;

    stream.msglen = 0;
    stream_init(&Sp, stream.buff, stream.bufflen);
    stream_byte_write(&Sp, header.version);
    stream_byte_write(&Sp, header.inverse_version);
    stream_2byte_write(&Sp, header.payload_type);
    stream_4byte_write(&Sp, header.payload_length);
    stream.msglen += stream_use_len(&Sp);

    return true;
}

bool DoipClient::DoipHeaderDecode(frameStream_t &stream, DoipHeader_t &header)
{
    stream_t Sp;

    stream_init(&Sp, stream.buff, stream.bufflen);
    header.version = stream_byte_read(&Sp);
    header.inverse_version = stream_byte_read(&Sp);
    header.payload_type = stream_2byte_read(&Sp);
    header.payload_length = stream_4byte_read(&Sp);

    return true;
}

bool DoipClient::DoipPayloadEncode(frameStream_t &stream, const char *payload, uint32_t payload_length)
{
    stream_t Sp;

    if (payload == nullptr) {
        return false;
    }

    stream_init(&Sp, stream.buff + stream.msglen, stream.bufflen - stream.msglen);
    stream_nbyte_write(&Sp, (unsigned char *)payload, payload_length);
    stream.msglen += stream_use_len(&Sp);

    return true;
}

bool DoipClient::DoipVehicleIdResponseDecode(frameStream_t &stream, VehicleIdentificationInfo_t &vid)
{
    stream_t Sp;

    stream_init(&Sp, stream.buff + DoipHeaderLen, stream.bufflen - DoipHeaderLen);
    stream_nbyte_read(&Sp, (unsigned char *)vid.vin, sizeof(vid.vin));
    vid.addr = stream_2byte_read(&Sp);
    stream_nbyte_read(&Sp, (unsigned char *)vid.eid, sizeof(vid.eid));
    stream_nbyte_read(&Sp, (unsigned char *)vid.gid, sizeof(vid.gid));

    return true;
}

int DoipClient::sendDoipMessage(QUdpSocket *udpSocket, const QHostAddress &host, quint16 port, DoipHeader_t &header, const char *payload, uint32_t payload_length)
{
    doipMessage_t msgInfo;

    if (udpSocket == nullptr) return -1;

    header.payload_length = payload_length;
    DoipHeaderEncode(sendStream, header);
    DoipPayloadEncode(sendStream, payload, header.payload_length);
    qint64 slen = udpSocket->writeDatagram((const char *)sendStream.buff, \
                             (qint16)sendStream.msglen, \
                             host,\
                             port);

    QDateTime current_time = QDateTime::currentDateTime();
    QString currentTime = current_time.toString("hh:mm:ss.zzz");
    msgInfo.time = currentTime;
    msgInfo.direction = "发送 ->";
    msgInfo.msgType = DoipMessageType(header.payload_type);
    msgInfo.msg.append((const char *)sendStream.buff, sendStream.msglen);
    msgInfo.state = slen == sendStream.msglen ? doipMessageNormal : doipMessageError;
    msgInfo.address = host;
    msgInfo.port = port;
    emit newDoipMessage(msgInfo);

    return slen;
}

int DoipClient::sendDoipMessage(const QHostAddress &host, quint16 port, DoipHeader_t &header, const char *payload, uint32_t payload_length)
{
    return 0;
}

QString DoipClient::DoipMessageType(qint32 type)
{
    switch (type) {
        case Generic_DoIP_header_NACK:
            return "通用DoIP报头NACK";
        case vehicleIdentificationRequest:
            return "车辆标识请求";
        case vehicleIdentificationEIDRequest:
            return "车辆标识请求包含EID";
        case vehicleIdentificationVINRequest:
            return "车辆标识请求包含VIN";
        case vehicleIdentificationResponseMessage:
            return "车辆标识响应";
        case routingActivationRequest:
            return "路由激活请求";
        case routingActivationResponse:
            return "路由激活响应";
        case diagnosticMessage:
            return "诊断请求";
        case diagnosticMessageAck:
            return "诊断请求正响应";
        case diagnosticMessageNack:
            return "诊断请求负响应";
        case aliveCheckRequest:
            return "活跃检测请求";
        case aliveCheckResponse:
            return "活跃检测应答";
        case DoipEntityStatusRequest:
            return "DOIP实体状态请求";
        case DoipEntityStatusResponse:
            return "DOIP实体状态应答";
        case DiagPowerModeInfoRequset:
            return "电源模式信息请求";
        case DiagPowerModeInfoResponse:
            return "电源模式信息应答";
    }

    return QString("未知/%1").arg(type);
}

QMap<QString, DoipClientConnect *> DoipClient::getDoipServerList()
{
    QMap<QString, DoipClientConnect *> doipServerList;

    for (int n = 0; n < doipClientConnectInfos.size(); n++) {
        DoipClientConnect *con = doipClientConnectInfos.at(n);
        if (con && con->isRouteActive()) {
            doipServerList.insert(con->getName(), con);
        }
    }

    return doipServerList;
}

void DoipClient::readPendingDatagrams()
{
    if (!udpSocket)
        return ;

    while (udpSocket->hasPendingDatagrams()) {
        QHostAddress address;
        quint16 port;
        recvStream.msglen = udpSocket->readDatagram((char *)recvStream.buff, recvStream.bufflen, &address, &port);
        if (recvStream.msglen > 0) {
            doipMessage_t msgInfo;
            DoipHeader_t header;
            DoipHeaderDecode(recvStream, header);

            QDateTime current_time = QDateTime::currentDateTime();
            QString currentTime = current_time.toString("hh:mm:ss.zzz");
            msgInfo.time = currentTime;
            msgInfo.direction = "接收 <-";
            msgInfo.msgType = DoipMessageType(header.payload_type);
            msgInfo.msg.append((const char *)recvStream.buff, recvStream.msglen);
            msgInfo.address = address;
            msgInfo.port = port;
            msgInfo.state = doipMessageNormal;
            if (header.payload_type == Generic_DoIP_header_NACK) {
                msgInfo.state = doipMessageWarning;
            }
            emit newDoipMessage(msgInfo);
            if (header.payload_type == vehicleIdentificationResponseMessage ||\
                header.payload_type == Generic_DoIP_header_NACK) {
                vehicleIdReqSendBtn->setEnabled(true);
                vehicleIdResponseLabel->setText(QString("<font color = green>响应时间(%1)ms</font>").arg(vehicleIdReqTime.msecsTo(QTime::currentTime())));
                vehicleIdRespTimer->stop();

                VehicleIdentificationInfo_t vid;
                QList<QStandardItem *> itemList;
                DoipVehicleIdResponseDecode(recvStream, vid);

                for (int n = 0; n < vehicleInfoModel->rowCount() || vehicleInfoModel->rowCount() == 0; n++) {
                    if ((vehicleInfoModel->item(n) && vehicleInfoModel->item(n)->text() != address.toString())\
                            || vehicleInfoModel->rowCount() == 0) {
                        QStandardItem *addrItem = new QStandardItem(address.toString());
                        itemList.append(addrItem);

                        QStandardItem *logcAddrItem = new QStandardItem(QString::number(vid.addr, 16));
                        logcAddrItem->setToolTip(logcAddrItem->text());
                        itemList.append(logcAddrItem);

                        char vinStr[32] = {0};
                        memcpy(vinStr, vid.vin, sizeof(vid.vin));
                        QStandardItem *vinItem = new QStandardItem(QString(vinStr));
                        vinItem->setToolTip(vinItem->text());
                        itemList.append(vinItem);

                        QStandardItem *eidItem = new QStandardItem(QString(QByteArray((const char *)vid.eid, sizeof(vid.eid)).toHex()));
                        eidItem->setToolTip(eidItem->text());
                        itemList.append(eidItem);

                        QStandardItem *gidItem = new QStandardItem(QString(QByteArray((const char *)vid.gid, sizeof(vid.gid)).toHex()));
                        gidItem->setToolTip(gidItem->text());
                        itemList.append(gidItem);

                        vid.address = address;
                        vehicleIdentificationInfos.append(vid);
                        vehicleInfoModel->appendRow(itemList);
                    }
                }
            }
        }
    }
}

void DoipClient::addDoipMessage(const doipMessage_t &val)
{
    emit newDoipMessage(val);
}

void DoipClient::RouteActiveState(DoipClientConnect *conn, bool state)
{
    qDebug() << conn << newDoipClientConn << (!state  ? "路由激活失败" : "路由激活成功");
    doipClientConnInfoIsChange = true;
    doipConnScanTimer->start();
    if (conn == newDoipClientConn) {
        newConBtn->setEnabled(true);
        doipConnRespTimer->stop();
        doipConnResponseLabel->setText(QString("<font color = green>连接激活时间(%1)ms</font>").arg(doipConnReqTime.msecsTo(QTime::currentTime())));
        for (int n = 0; n < vehicleIdentificationInfos.size(); n++) {
            qDebug() << vehicleIdentificationInfos.at(n).address << " " << conn->getServerHostAddress();
            qDebug() << vehicleIdentificationInfos.at(n).addr << " " << conn->getLogicTargetAddress();
            if (vehicleIdentificationInfos.at(n).address == conn->getServerHostAddress() && \
                vehicleIdentificationInfos.at(n).addr == conn->getLogicTargetAddress()) {
                char vinStr[32] = {0};
                memcpy(vinStr, vehicleIdentificationInfos.at(n).vin, sizeof(vehicleIdentificationInfos.at(n).vin));
                conn->setServerVin(vinStr);
                qDebug() << QString(vinStr);
                qDebug() << conn->getServerVin();
            }
        }
        doipClientConnectInfos.append(conn);
        newDoipClientConn= nullptr;
    }
    QMap<QString, DoipClientConnect *> conMap = getDoipServerList();
    emit doipServerChange(conMap);
}

void DoipClient::diagnosisResponse(DoipClientConnect *doipConn, DoipClientConnect::sendDiagFinishState state, QByteArray &response)
{
    if (doipConn == udsDoipClientConn) {
        udsDoipClientConn = nullptr;
        diagRespMsgEdit->setPlainText(response.toHex(' '));
    }
    emit diagnosisFinishSignal(doipConn, state, response);
}

DoipClientConnect::DoipClientConnect(QObject *parent)
{
    Q_UNUSED(parent);

    sendStream.buff = new uint8_t[40960];
    sendStream.bufflen = 40960;
    sendStream.msglen = 0;

    recvStream.buff = new uint8_t[40960];
    recvStream.bufflen = 40960;
    recvStream.msglen = 0;

    this->version = 0;
    this->sourceAddr = 0;
    this->hostPort = 0;
    this->sourceAddr = 0;
    this->funcTargetAddr = 0;
    this->logicTargetAddr = 0;
    this->reconnect = false;
    this->slotIsConnect = false;
    TcpSocket = new QTcpSocket(this);
    clientIsActive = false;
    activeTimer = new QTimer(this);
}

DoipClientConnect::DoipClientConnect(uint32_t recvlen, uint32_t sendlen)
{
    recvlen = recvlen > 0 ? recvlen : 40960;
    sendlen = sendlen > 0 ? sendlen : 40960;

    qDebug() << "recvlen:" << recvlen << "sendlen:" << sendlen;

    sendStream.buff = new uint8_t[recvlen];
    sendStream.bufflen = recvlen;
    sendStream.msglen = 0;

    recvStream.buff = new uint8_t[sendlen];
    recvStream.bufflen = sendlen;
    recvStream.msglen = 0;

    this->version = 0;
    this->sourceAddr = 0;
    this->hostPort = 0;
    this->sourceAddr = 0;
    this->funcTargetAddr = 0;
    this->logicTargetAddr = 0;
    this->reconnect = false;
    this->slotIsConnect = false;
    TcpSocket = new QTcpSocket(this);
    clientIsActive = false;
    activeTimer = new QTimer(this);
}

DoipClientConnect::~DoipClientConnect()
{
    delete [] sendStream.buff;
    delete [] recvStream.buff;
}

void DoipClientConnect::backupCacheInit()
{
    QSettings operationSettings(serviceBackCacheDir + "doipConnectOperationSettings.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));
}

void DoipClientConnect::operationCache()
{
    QSettings operationSettings(serviceBackCacheDir + "doipConnectOperationSettings.ini", QSettings::IniFormat);
    operationSettings.setIniCodec(QTextCodec::codecForName("utf-8"));

    operationSettings.sync();
}

bool DoipClientConnect::routeActiveRequest(QHostAddress addr, quint16 port, quint8 ver, quint16 sa, quint16 timeout)
{
    if (TcpSocket == nullptr) {
        return false;
    }
    qDebug() <<  this << "TcpSocket->state(): " << TcpSocket->state();
    if (TcpSocket->state() != QAbstractSocket::UnconnectedState || \
        activeTimer->isActive()) {
        qDebug() <<  this << "Doip client connecting...";
        return false;
    }
    TcpSocket->abort();
    TcpSocket->connectToHost(addr, port);
    this->version = ver;
    this->sourceAddr = sa;
    this->hostAddr = addr;
    this->hostPort = port;

    if (!this->slotIsConnect) {
        this->slotIsConnect = true;
    }
    else {
        return true;
    }

    connect(TcpSocket, &QTcpSocket::connected, [this, addr, port, ver, timeout] {
        qDebug() << this << "connected";
        doipMessage_t msgInfo;
        DoipHeader_t header;
        stream_t Sp;

        header.version = ver;
        header.inverse_version = ~ver;
        header.payload_type = routingActivationRequest;
        header.payload_length = 7;
        DoipClient::DoipHeaderEncode(sendStream, header);
        stream_init(&Sp, sendStream.buff + DoipHeaderLen, sendStream.bufflen - DoipHeaderLen);
        stream_2byte_write(&Sp, this->sourceAddr);
        stream_byte_write(&Sp, 0);
        stream_4byte_write(&Sp, 0);
        sendStream.msglen += stream_use_len(&Sp);

        QDateTime current_time = QDateTime::currentDateTime();
        QString currentTime = current_time.toString("hh:mm:ss.zzz");
        msgInfo.time = currentTime;
        msgInfo.direction = "发送 ->";
        msgInfo.msgType = DoipClient::DoipMessageType(header.payload_type);
        msgInfo.msg.append((const char *)sendStream.buff, sendStream.msglen);
        msgInfo.address = addr;
        msgInfo.port = port;
        msgInfo.state = doipMessageNormal;
        emit newDoipMessage(msgInfo);
        TcpSocket->write((const char *)sendStream.buff, sendStream.msglen);
        activeTimer->setInterval(timeout);
        activeTimer->start();
        clientIsActive = false;
    });

    connect(TcpSocket, &QTcpSocket::disconnected, [this]{
        qDebug() <<  this << "disconnected";
        clientIsActive = false;
        TcpSocket->abort();
        activeTimer->stop();
        emit routeActiveStateChange(this, false);
    });

    connect(TcpSocket, &QTcpSocket::readyRead, [addr, port, this]{
        if (TcpSocket->bytesAvailable() <= 0)
            return;
        recvStream.msglen = TcpSocket->read((char *)recvStream.buff, recvStream.bufflen);
        downTotalByte += recvStream.msglen;
        transferAverageSpeed(downPreTime, downTotalByte, downAverageByte);

        doipMessage_t recvMsgInfo;
        DoipHeader_t header;

        for (;recvStream.msglen > 0;) {
            DoipClient::DoipHeaderDecode(recvStream, header);
            QDateTime current_time = QDateTime::currentDateTime();
            QString currentTime = current_time.toString("hh:mm:ss.zzz");
            recvMsgInfo.state = doipMessageNormal;
            recvMsgInfo.time = currentTime;
            recvMsgInfo.direction = "接收 <-";
            recvMsgInfo.msgType = DoipClient::DoipMessageType(header.payload_type);
            recvMsgInfo.msg.clear();
            if (recvStream.msglen >= header.payload_length + DoipHeaderLen) {
                recvMsgInfo.msg.append((const char *)recvStream.buff, header.payload_length + DoipHeaderLen);
            }
            else {
                recvMsgInfo.msg.append((const char *)recvStream.buff, recvStream.msglen);
                recvMsgInfo.state = doipMessageWarning;
            }
            recvMsgInfo.address = addr;
            recvMsgInfo.port = port;
            if (header.payload_type == routingActivationResponse) {
                activeTimer->stop();
                stream_t Sp;
                quint8 code;

                stream_init(&Sp, recvStream.buff + DoipHeaderLen, recvStream.bufflen - DoipHeaderLen);
                this->sourceAddr = stream_2byte_read(&Sp);
                this->logicTargetAddr = stream_2byte_read(&Sp);
                code = stream_byte_read(&Sp);
                if (code != 0x10) {
                    recvMsgInfo.state = doipMessageWarning;
                }
                stream_nbyte_read(&Sp, this->iso, sizeof(this->iso));
                stream_nbyte_read(&Sp, this->oem, sizeof(this->oem));
                if (code != 0x10) {
                    emit routeActiveStateChange(this, false);
                }
                else {
                    clientIsActive = true;
                    emit routeActiveStateChange(this, true);
                }
                emit newDoipMessage(recvMsgInfo);
            }
            else if (header.payload_type == diagnosticMessageAck) {
                emit newDoipMessage(recvMsgInfo);
            }
            else if (header.payload_type == diagnosticMessageNack) {
                QByteArray response;

                recvMsgInfo.state = doipMessageWarning;
                emit diagnosisFinish(this, nackFinish, response);
                emit newDoipMessage(recvMsgInfo);
            }
            else if (header.payload_type == diagnosticMessage) {
                QByteArray response;
                stream_t Sp;

                stream_init(&Sp, recvStream.buff + DoipHeaderLen, recvStream.bufflen - DoipHeaderLen);
                quint16 logicTargetAddr = stream_2byte_read(&Sp);
                quint16 sourceAddr = stream_2byte_read(&Sp);

                if (sourceAddr != this->sourceAddr ||\
                    logicTargetAddr != this->logicTargetAddr) {
                    recvMsgInfo.state = doipMessageError;
                }
                response.append((const char *)recvStream.buff + DoipHeaderLen + 4, header.payload_length - 4);

                emit diagnosisFinish(this, normalFinish, response);
                emit newDoipMessage(recvMsgInfo);
            }
            else if (header.payload_type == aliveCheckRequest) {
                doipMessage_t sendMsgInfo;
                stream_t Sp;

                header.payload_type = aliveCheckResponse;
                header.payload_length = 2;
                DoipClient::DoipHeaderEncode(sendStream, header);
                stream_init(&Sp, recvStream.buff + DoipHeaderLen, recvStream.bufflen - DoipHeaderLen);
                stream_2byte_write(&Sp, this->sourceAddr);
                sendStream.msglen += stream_use_len(&Sp);

                QDateTime current_time = QDateTime::currentDateTime();
                QString currentTime = current_time.toString("hh:mm:ss.zzz");
                sendMsgInfo.time = currentTime;
                sendMsgInfo.direction = "发送 ->";
                sendMsgInfo.msgType = DoipClient::DoipMessageType(header.payload_type);
                sendMsgInfo.msg.append((const char *)recvStream.buff, sendStream.msglen);
                sendMsgInfo.address = addr;
                sendMsgInfo.port = port;
                sendMsgInfo.state = doipMessageNormal;
                emit newDoipMessage(recvMsgInfo);
                emit newDoipMessage(sendMsgInfo);
                TcpSocket->write((const char *)recvStream.buff, sendStream.msglen);
            }
            if (recvStream.msglen >= header.payload_length + DoipHeaderLen) {
                recvStream.msglen -= (header.payload_length + DoipHeaderLen);
                memmove(recvStream.buff, \
                        recvStream.buff + header.payload_length + DoipHeaderLen,\
                        recvStream.msglen);
            }
            else {
                recvStream.msglen = 0;
            }
        }
    });

    connect(activeTimer, &QTimer::timeout, this, [this] {
        qDebug() <<  this << "activeTimer";
        clientIsActive = false;
        TcpSocket->abort();
        activeTimer->stop();
        emit routeActiveStateChange(this, false);
    });

    return true;
}

quint32 DoipClientConnect::diagnosisRequest(const char *buf, quint32 len, quint16 ta, bool supression, quint16 timeout)
{
    DoipHeader_t header;
    stream_t Sp;

    if (!buf) return 0;

    if (!isRouteActive()) {
        /* doip未进行路由未激活 */
        return 0;
    }

    /* doip协议封装 */
    header.version = version;
    header.inverse_version = ~version;
    header.payload_type = diagnosticMessage;
    header.payload_length = len + 4;
    DoipClient::DoipHeaderEncode(sendStream, header);
    stream_init(&Sp, sendStream.buff + DoipHeaderLen, sendStream.bufflen - DoipHeaderLen);
    stream_2byte_write(&Sp, this->sourceAddr);
    stream_2byte_write(&Sp, ta);
    sendStream.msglen += 4;
    DoipClient::DoipPayloadEncode(sendStream, buf, len);
    qint64 slen = TcpSocket->write((const char *)sendStream.buff, sendStream.msglen);
    upTotalByte += slen;

    transferAverageSpeed(upPreTime, upTotalByte, upAverageByte);

    /* 记录发送的doip报文 */
    doipMessage_t sendMsgInfo;

    QDateTime current_time = QDateTime::currentDateTime();
    QString currentTime = current_time.toString("hh:mm:ss.zzz");
    sendMsgInfo.time = currentTime;
    sendMsgInfo.direction = "发送 ->";
    sendMsgInfo.msgType = DoipClient::DoipMessageType(header.payload_type);
    sendMsgInfo.msg.append((const char *)sendStream.buff, sendStream.msglen);
    sendMsgInfo.address = this->hostAddr;
    sendMsgInfo.port = this->hostPort;
    sendMsgInfo.state = doipMessageNormal;

    emit newDoipMessage(sendMsgInfo);

    return slen;
}

QHostAddress DoipClientConnect::getServerHostAddress()
{
    return this->hostAddr;
}

quint16 DoipClientConnect::getServerHostPort()
{
    return this->hostPort;
}

quint8 DoipClientConnect::getDoipVersion()
{
    return this->version;
}

quint16 DoipClientConnect::getSourceAddress()
{
    return this->sourceAddr;
}

quint16 DoipClientConnect::getFuncTargetAddress()
{
    return this->funcTargetAddr;
}

quint16 DoipClientConnect::getLogicTargetAddress()
{
    return this->logicTargetAddr;
}

QString DoipClientConnect::getServerVin()
{
    return this->vin;
}

QString DoipClientConnect::getName()
{
    this->name.clear();
    this->name = QString(getServerHostAddress().toString() + "/" + QString::number(getLogicTargetAddress(), 16));

    return this->name;
}

void DoipClientConnect::setReconnect(bool reconnect)
{
    this->reconnect = reconnect;
}

bool DoipClientConnect::isReconnect()
{
    return reconnect;
}

bool DoipClientConnect::setServerVin(QString vin)
{
    this->vin = vin;
    return true;
}

bool DoipClientConnect::isRouteActive()
{
    return clientIsActive;
}

void DoipClientConnect::disconnect()
{
    if (clientIsActive) {
        TcpSocket->abort();
        clientIsActive = false;
    }
}

int DoipClientConnect::transferAverageSpeed(QDateTime &preTime, qint32 &totalByte, qint32 &averageByte)
{
    QDateTime currDateTime = QDateTime::currentDateTime();
    int intervalSecs = preTime.secsTo(currDateTime);
    if (intervalSecs > 2) {
        if (intervalSecs > 3) {
            averageByte = 0;
            totalByte = 0;
            preTime = currDateTime;
        } else {
            totalByte -= (averageByte * intervalSecs);
            preTime = preTime.addSecs(intervalSecs);
            averageByte = totalByte / intervalSecs;
            if (averageByte < 0) {
                averageByte = 0;
                totalByte = 0;
                preTime = currDateTime;
            }
        }
    }
    else if (totalByte < 0) {
        averageByte = 0;
        totalByte = 0;
        preTime = currDateTime;
    }

    return averageByte;
}
