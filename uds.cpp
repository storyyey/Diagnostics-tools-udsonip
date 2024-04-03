#include <QHeaderView>
#include <QDebug>
#include "uds.h"
#include "stream.h"


UDS::UDS(QObject *parent) : QObject(parent)
{
    QMap<quint8, QString> subDesc;

    can = new CAN();
    connect(can, SIGNAL(udsResponse(QString, int, QByteArray)),\
            this, SLOT(canResponseHandler(QString, int, QByteArray)));

    modifyRowIndex = -1;
    manualSendRowIndex = -1;
    serviceListTaskTimer = new QTimer();
    connect(serviceListTaskTimer, &QTimer::timeout, this, [this]{
        if (serviceActive()) {
            if (!highPerformanceEnable) {
                /* 高性能模式下不进行UI操作 */
                if (serviceActiveTask.indexRow < serviceSetModel->rowCount() - 1) {
                    /* 未到最后一个测试项，进度条滚动显示下一个未测试的项 */
                    serviceSetView->scrollTo(serviceSetModel->index(serviceActiveTask.indexRow + 1, 0));
                }
                else {
                    /* 已经到了最后一个测试项 */
                    serviceSetView->scrollTo(serviceSetModel->index(serviceActiveTask.indexRow, 0));
                }
            }
            while (serviceActive()) {
                /* while 遍历找到一个使能的测试项 */
                QCheckBox* enableCheck = qobject_cast<QCheckBox*>(serviceSetView->indexWidget(\
                                             serviceSetModel->index(serviceActiveTask.indexRow, udsViewItemEnable)));
                /* 测试项使能，执行这个测试项 */
                if (enableCheck && enableCheck->checkState() == Qt::Checked) {
#if 1
                    QList<QStandardItem *> itemList;
                    for (int nn = 0; nn < serviceSetModel->columnCount(); nn++) {
                        QStandardItem *item = serviceSetModel->item(serviceActiveTask.indexRow, nn);
                        itemList.append(item);
                    }
                    setActiveServiceFlicker(itemList);
#endif
                    if (sendServiceRequest(serviceActiveTask.indexRow, serviceActiveTask.channelName) || true) {
                        /* 发送成功了才能退出while循环 */
                        serviceItem data;
                        getServiceItemData(serviceActiveTask.indexRow, data);
                        /* 启动一下定时器 */
                        int tmms = data.timeout > 0 ? data.timeout : P2ServerMax;
                        serviceActiveTask.responseTimer.setInterval(tmms);
                        serviceActiveTask.responseTimer.start();
                        break;
                    }
                }
                else if (serviceActiveTask.indexRow == serviceSetModel->rowCount() - 1) {
                    /* 最后一个任务非使能的，在这里结束所有任务 */
                    serverSetFinishHandle();
                }
                else {
                    /* 还有测试项未执行, 遍历表中的下一个测试项 */
                    serviceActiveTask.indexRow++;
                }
            }
        }
        /* 发送定时器停止 */
        serviceListTaskTimer->stop();
    });

    connect(&serviceActiveTask.responseTimer, &QTimer::timeout, this, [this]{
        QByteArray response;
        diagnosisResponseHandle(serviceActiveTask.channelName, DoipClientConnect::TimeoutFinish, response);
    });

    serviceMap.insert(QString::asprintf("DiagnosticSessionControl(0x%02X)", DiagnosticSessionControl), DiagnosticSessionControl);
    serviceMap.insert(QString::asprintf("ECUReset (0x%02X)", ECUReset), ECUReset);
    serviceMap.insert(QString::asprintf("SecurityAccess (0x%02X)", SecurityAccess), SecurityAccess);
    serviceMap.insert(QString::asprintf("CommunicationControl (0x%02X)", CommunicationControl), CommunicationControl);
    serviceMap.insert(QString::asprintf("TesterPresent (0x%02X)", TesterPresent), TesterPresent);
    serviceMap.insert(QString::asprintf("AccessTimingParameter (0x%02X)", AccessTimingParameter), AccessTimingParameter);
    serviceMap.insert(QString::asprintf("SecuredDataTransmission (0x%02X)", SecuredDataTransmission), SecuredDataTransmission);
    serviceMap.insert(QString::asprintf("ControlDTCSetting (0x%02X)", ControlDTCSetting), ControlDTCSetting);
    serviceMap.insert(QString::asprintf("ResponseOnEvent (0x%02X)", ResponseOnEvent), ResponseOnEvent);
    serviceMap.insert(QString::asprintf("LinkControl (0x%02X)", LinkControl), LinkControl);
    serviceMap.insert(QString::asprintf("ReadDataByIdentifier (0x%02X)", ReadDataByIdentifier), ReadDataByIdentifier);
    serviceMap.insert(QString::asprintf("ReadMemoryByAddress (0x%02X)", ReadMemoryByAddress), ReadMemoryByAddress);
    serviceMap.insert(QString::asprintf("ReadScalingDataByIdentifier (0x%02X)", ReadScalingDataByIdentifier), ReadScalingDataByIdentifier);
    serviceMap.insert(QString::asprintf("ReadDataByPeriodicIdentifier (0x%02X)", ReadDataByPeriodicIdentifier), ReadDataByPeriodicIdentifier);
    serviceMap.insert(QString::asprintf("DynamicallyDefineDataIdentifier (0x%02X)", DynamicallyDefineDataIdentifier), DynamicallyDefineDataIdentifier);
    serviceMap.insert(QString::asprintf("WriteDataByIdentifier (0x%02X)", WriteDataByIdentifier), WriteDataByIdentifier);
    serviceMap.insert(QString::asprintf("WriteMemoryByAddress (0x%02X)", WriteMemoryByAddress), WriteMemoryByAddress);
    serviceMap.insert(QString::asprintf("ClearDiagnosticInformation (0x%02X)", ClearDiagnosticInformation), ClearDiagnosticInformation);
    serviceMap.insert(QString::asprintf("ReadDTCInformation (0x%02X)", ReadDTCInformation), ReadDTCInformation);
    serviceMap.insert(QString::asprintf("InputOutputControlByIdentifier (0x%02X)", InputOutputControlByIdentifier), InputOutputControlByIdentifier);
    serviceMap.insert(QString::asprintf("RoutineControl (0x%02X)", RoutineControl), RoutineControl);
    serviceMap.insert(QString::asprintf("RequestDownload (0x%02X)", RequestDownload), RequestDownload);
    serviceMap.insert(QString::asprintf("RequestUpload (0x%02X)", RequestUpload), RequestUpload);
    serviceMap.insert(QString::asprintf("TransferData (0x%02X)", TransferData), TransferData);
    serviceMap.insert(QString::asprintf("RequestTransferExit (0x%02X)", RequestTransferExit), RequestTransferExit);
    serviceMap.insert(QString::asprintf("RequestFileTransfer (0x%02X)", RequestFileTransfer), RequestFileTransfer);

    QMap<QString, quint8>::Iterator serviceMapIt;
    for (serviceMapIt = serviceMap.begin(); serviceMapIt != serviceMap.end(); ++serviceMapIt) {
        serviceDescMap.insert(serviceMapIt.value(), serviceMapIt.key());
    }

    QMap<QString, quint8>::Iterator subMapIt;

    QMap<QString, quint8> sid10sub;
    sid10sub.insert("defaultSession(0x01)", 0x01);
    sid10sub.insert("ProgrammingSession(0x02)", 0x02);
    sid10sub.insert("extendedDiagnosticSession(0x03)", 0x03);
    sid10sub.insert("safetySystemDiagnosticSession(0x04)", 0x04);
    serviceSubMap.insert(0x10, sid10sub);
    for (subMapIt = sid10sub.begin(); subMapIt != sid10sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x10, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid11sub;
    sid11sub.insert("hardReset(0x01)", 0x01);
    sid11sub.insert("keyOffOnReset(0x02)", 0x02);
    sid11sub.insert("softReset(0x03)", 0x03);
    sid11sub.insert("enableRapidPowerShutDown(0x04)", 0x04);
    sid11sub.insert("disableRapidPowerShutDown(0x05)", 0x05);
    serviceSubMap.insert(0x11, sid11sub);
    for (subMapIt = sid11sub.begin(); subMapIt != sid11sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x11, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid27sub;
    for (quint8 sub = 1; sub < 0xff; sub++) {
        if (sub % 2 == 0) {
            sid27sub.insert(QString("RequestKey(0x%1)").arg(QString(QByteArray(1, (char)sub).toHex())), sub);
        }
        else {
            sid27sub.insert(QString("RequestSeed(0x%1)").arg(QString(QByteArray(1, (char)sub).toHex())), sub);
        }
    }
    serviceSubMap.insert(0x27, sid27sub);
    for (subMapIt = sid27sub.begin(); subMapIt != sid27sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x27, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid28sub;
    sid28sub.insert("enableRxAndTx(0x00)", 0x00);
    sid28sub.insert("enableRxAndDisableTx(0x01)", 0x01);
    sid28sub.insert("disableRxAndEnableTx(0x02)", 0x02);
    sid28sub.insert("disableRxAndTx(0x03)", 0x03);
    sid28sub.insert("enableRxAndDisableTxWithEnhancedAddressInformation(0x04)", 0x04);
    sid28sub.insert("enableRxAndTxWithEnhancedAddressInformation(0x05)", 0x05);
    serviceSubMap.insert(0x28, sid28sub);
    for (subMapIt = sid28sub.begin(); subMapIt != sid28sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x28, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid83sub;
    sid83sub.insert("readExtendedTimingParameterSet(0x01)", 0x01);
    sid83sub.insert("setTimingParametersToDefaultValues(0x02)", 0x02);
    sid83sub.insert("readCurrentlyActiveTimingParameters(0x03)", 0x03);
    sid83sub.insert("setTimingParametersToGivenValues(0x04)", 0x04);
    serviceSubMap.insert(0x83, sid83sub);
    for (subMapIt = sid83sub.begin(); subMapIt != sid83sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x83, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid85sub;
    sid85sub.insert("on(0x01)", 0x01);
    sid85sub.insert("off(0x02)", 0x02);
    serviceSubMap.insert(0x85, sid85sub);
    for (subMapIt = sid85sub.begin(); subMapIt != sid85sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x85, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid86sub;
    sid86sub.insert("stopResponseOnEvent(0x00)", 0x00);
    sid86sub.insert("onDTCStatusChange(0x01)", 0x01);
    sid86sub.insert("onTimerInterrupt(0x02)", 0x02);
    sid86sub.insert("onChangeOfDataIdentifier(0x03)", 0x03);
    sid86sub.insert("reportActivatedEvents(0x04)", 0x04);
    sid86sub.insert("startResponseOnEvent(0x05)", 0x05);
    sid86sub.insert("clearResponseOnEvent(0x06)", 0x06);
    sid86sub.insert("onComparisonOfValues(0x07)", 0x07);
    serviceSubMap.insert(0x86, sid86sub);
    for (subMapIt = sid86sub.begin(); subMapIt != sid86sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x86, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid87sub;
    sid87sub.insert("verifyModeTransitionWithFixedParameter(0x01)", 0x01);
    sid87sub.insert("verifyModeTransitionWithSpecificParameter(0x02)", 0x02);
    sid87sub.insert("transitionMode(0x03)", 0x03);
    serviceSubMap.insert(0x87, sid87sub);
    for (subMapIt = sid87sub.begin(); subMapIt != sid87sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x87, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid2csub;
    sid2csub.insert("defineByIdentifier(0x01)", 0x01);
    sid2csub.insert("defineByMemoryAddress(0x02)", 0x02);
    sid2csub.insert("clearDynamicallyDefinedDataIdentifier(0x03)", 0x03);
    serviceSubMap.insert(0x2c, sid2csub);
    for (subMapIt = sid2csub.begin(); subMapIt != sid2csub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x2c, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid31sub;
    sid31sub.insert("startRoutine(0x01)", 0x01);
    sid31sub.insert("stopRoutine(0x02)", 0x02);
    sid31sub.insert("requestRoutineResults(0x03)", 0x03);
    serviceSubMap.insert(0x31, sid31sub);
    for (subMapIt = sid31sub.begin(); subMapIt != sid31sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x31, subDesc);
    subDesc.clear();

    QMap<QString, quint8> sid19sub;
    sid19sub.insert("reportNumberOfDTCByStatusMask(0x01)", 0x01);
    sid19sub.insert("reportDTCByStatusMask(0x02)", 0x02);
    sid19sub.insert("reportDTCSnapshotIdentification(0x03)", 0x03);
    sid19sub.insert("reportDTCSnapshotIdentification(0x04)", 0x04);
    sid19sub.insert("reportDTCSnapshotRecordByDTCNumber(0x05)", 0x05);
    sid19sub.insert("reportDTCExtDataRecordByDTCNumber(0x06)", 0x06);
    sid19sub.insert("reportNumberOfDTCBySeverityMaskRecord(0x07)", 0x07);
    sid19sub.insert("reportDTCBySeverityMaskRecord(0x08)", 0x08);
    sid19sub.insert("reportSeverityInformationOfDTC(0x09)", 0x09);
    sid19sub.insert("reportSupportedDTC(0x0A)", 0x0A);
    sid19sub.insert("reportFirstTestFailedDTC(0x0B)", 0x0B);
    sid19sub.insert("reportFirstConfirmedDTC(0x0C)", 0x0C);
    sid19sub.insert("reportMirrorMemoryDTCByStatusMask(0x0f)", 0x0f);
    sid19sub.insert("reportMirrorMemoryDTCExtDataRecordByDTCNumber(0x10)", 0x10);
    sid19sub.insert("reportNumberOfMirrorMemoryDTCByStatusMask(0x11)", 0x11);
    sid19sub.insert("reportNumberOfEmissionsOBDDTCByStatusMask(0x12)", 0x12);
    sid19sub.insert("reportEmissionsOBDDTCByStatusMask(0x13)", 0x13);
    sid19sub.insert("reportDTCFaultDetectionCounter(0x14)", 0x14);
    sid19sub.insert("reportDTCWithPermanentStatus(0x15)", 0x15);
    sid19sub.insert("reportDTCExtDataRecordByRecordNumber(0x16)", 0x16);
    sid19sub.insert("reportUserDefMemoryDTCByStatusMask(0x17)", 0x17);
    sid19sub.insert("reportUserDefMemoryDTCSnapshotRecordByDTCNumber(0x18)", 0x18);
    sid19sub.insert("reportUserDefMemoryDTCExtDataRecordByDTCNumber(0x19)", 0x19);
    sid19sub.insert("reportUserDefMemoryDTCExtDataRecordByDTCNumber(0x42)", 0x42);
    sid19sub.insert("reportWWHOBDDTCWithPermanentStatus(0x55)", 0x55);
    serviceSubMap.insert(0x19, sid19sub);
    for (subMapIt = sid19sub.begin(); subMapIt != sid19sub.end(); ++subMapIt) {
        subDesc.insert(subMapIt.value(), subMapIt.key());
    }
    serviceSubDescMap.insert(0x19, subDesc);
    subDesc.clear();

    responseExpectMap.insert("default setting", notSetResponseExpect);
    responseExpectMap.insert("negative response", negativeResponseExpect);
    responseExpectMap.insert("positive response", positiveResponseExpect);
    responseExpectMap.insert("no response", noResponseExpect);
    responseExpectMap.insert("regex match", responseRegexMatch);
    responseExpectMap.insert("regex mismatch", responseRegexMismatch);
    QMap<QString, UDS::serviceResponseExpect>::Iterator responseExpect;
    for (responseExpect = responseExpectMap.begin();\
         responseExpect != responseExpectMap.end(); ++responseExpect) {
        responseExpectDesc.insert(responseExpect.value(), responseExpect.key());
    }

    positiveResponseCodeMap.insert("generalReject(0x10)", 0x10);
    positiveResponseCodeMap.insert("serviceNotSupported(0x11)", 0x11);
    positiveResponseCodeMap.insert("sub-functionNotSupported(0x12)", 0x12);
    positiveResponseCodeMap.insert("incorrectMessageLengthOrInvalidFormat(0x13)", 0x13);
    positiveResponseCodeMap.insert("busyRepeatRequest(0x21)", 0x21);
    positiveResponseCodeMap.insert("conditionsNotCorrect(0x22)", 0x22);
    positiveResponseCodeMap.insert("requestSequenceError(0x24)", 0x24);
    positiveResponseCodeMap.insert("noResponseFromSubnetComponent(0x25)", 0x25);
    positiveResponseCodeMap.insert("FailurePreventsExecutionOfRequestedAction(0x26)", 0x26);
    positiveResponseCodeMap.insert("requestOutOfRange(0x31)", 0x31);
    positiveResponseCodeMap.insert("securityAccessDenied(0x33)", 0x33);
    positiveResponseCodeMap.insert("invalidKey(0x35)", 0x35);
    positiveResponseCodeMap.insert("exceedNumberOfAttempts(0x36)", 0x36);
    positiveResponseCodeMap.insert("requiredTimeDelayNotExpired(0x37)", 0x37);
    positiveResponseCodeMap.insert("uploadDownloadNotAccepted(0x70)", 0x70);
    positiveResponseCodeMap.insert("transferDataSuspended(0x71)", 0x71);
    positiveResponseCodeMap.insert("generalProgrammingFailure(0x72)", 0x72);
    positiveResponseCodeMap.insert("wrongBlockSequenceCounter(0x73)", 0x73);
    positiveResponseCodeMap.insert("requestCorrectlyReceived-ResponsePending(0x78)", 0x78);
    positiveResponseCodeMap.insert("sub-functionNotSupportedInActiveSession(0x7E)", 0x7E);
    positiveResponseCodeMap.insert("serviceNotSupportedInActiveSession(0x7F)", 0x7F);
    positiveResponseCodeMap.insert("rpmTooHigh(0x81)", 0x81);
    positiveResponseCodeMap.insert("rpmTooLow(0x82)", 0x82);
    positiveResponseCodeMap.insert("engineIsRunning(0x83)", 0x83);
    positiveResponseCodeMap.insert("engineIsNotRunning(0x84)", 0x84);
    positiveResponseCodeMap.insert("engineRunTimeTooLow(0x85)", 0x85);
    positiveResponseCodeMap.insert("temperatureTooHigh(0x86)", 0x86);
    positiveResponseCodeMap.insert("temperatureTooLow(0x87)", 0x87);
    positiveResponseCodeMap.insert("vehicleSpeedTooHigh(0x88)", 0x88);
    positiveResponseCodeMap.insert("vehicleSpeedTooLow(0x89)", 0x89);
    positiveResponseCodeMap.insert("throttle/PedalTooHigh(0x8A)", 0x8A);
    positiveResponseCodeMap.insert("throttle/PedalTooLow(0x8B)", 0x8B);
    positiveResponseCodeMap.insert("transmissionRangeNotInNeutral(0x8C)", 0x8C);
    positiveResponseCodeMap.insert("transmissionRangeNotInGear(0x8D)", 0x8D);
    positiveResponseCodeMap.insert("brakeSwitch(es)NotClosed(0x8F)", 0x8F);
    positiveResponseCodeMap.insert("shifterLeverNotInPark(0x90)", 0x90);
    positiveResponseCodeMap.insert("torqueConverterClutchLocked(0x91)", 0x91);
    positiveResponseCodeMap.insert("voltageTooHigh(0x92)", 0x92);
    positiveResponseCodeMap.insert("voltageTooLow(0x93)", 0x93);

    QMap<QString, quint8>::Iterator negativeResponseCode;
    for (negativeResponseCode = positiveResponseCodeMap.begin();\
         negativeResponseCode != positiveResponseCodeMap.end(); ++negativeResponseCode) {
        positiveResponseCodeDesc.insert(negativeResponseCode.value(), negativeResponseCode.key());
    }

    serviceItemStateColor.insert(serviceItemAll, "#FFFAFA");
    serviceItemStateColor.insert(serviceItemNotTest, "#CDC9C9");
    serviceItemStateColor.insert(serviceItemTestPass, "#228B22");
    serviceItemStateColor.insert(serviceItemTestFail, "#EE3B3B");
    serviceItemStateColor.insert(serviceItemTestWarn, "#FFFF00");
    serviceItemStateColor.insert(serviceItemTestProcess, "#00BFFF");

    QFileInfo dtcfileInfo("./dictionaryEncoding/DTCDictionary/DTCDictionary.json");
    QFile dtcfile("./dictionaryEncoding/DTCDictionary/DTCDictionary.json");
    if (dtcfileInfo.isFile()) {
        dtcfile.open(QIODevice::ReadOnly);
        QByteArray linedata;
        do {
            linedata.clear();
            linedata = dtcfile.readLine();
            if (linedata.size()) {
                QString config = QString(linedata).toUtf8().data();
                addDtcDictionaryItem(config);
            }
        } while (linedata.size() > 0);
        dtcfile.close();
    }
    QFileInfo DIDfileInfo("./dictionaryEncoding/DIDDictionary/DIDDictionary.json");
    QFile DIDfile("./dictionaryEncoding/DIDDictionary/DIDDictionary.json");
    if (DIDfileInfo.isFile()) {
        DIDfile.open(QIODevice::ReadOnly);
        QByteArray linedata;
        do {
            linedata.clear();
            linedata = DIDfile.readLine();
            if (linedata.size()) {
                QString config = QString(linedata).toUtf8().data();
                addDidDictionaryItem(config);
            }
        } while (linedata.size() > 0);
        DIDfile.close();
    }

    serviceSetWidget = serviceConfigWidget();
    //serviceSetWidget->setWindowFlags(serviceSetWidget->windowFlags() | Qt::WindowStaysOnTopHint);

    QTimer *flickerTimer = new QTimer();
    flickerTimer->setInterval(10);
    flickerTimer->start();
    connect(flickerTimer, &QTimer::timeout, this, [this]{
        if (highPerformanceEnable) {
            /* 高性能模式下直接退出 */
            return ;
        }
        if (activeServiceFlicker.isvalid && serviceActive()) {
            for (int ListIndex = 0; ListIndex < activeServiceFlicker.Item.size(); ListIndex++) {
                QStandardItem *item = activeServiceFlicker.Item.at(ListIndex);
                if (item) {
                    item->setBackground(QBrush(((activeServiceFlicker.statusLevel / 10 + 155) << 8) + 255) );
                }
            }
            if (activeServiceFlicker.direct) {
                if (activeServiceFlicker.statusLevel >= 900) {
                    activeServiceFlicker.direct = false;
                }
                activeServiceFlicker.statusLevel += 10;
            }
            else {
                if (activeServiceFlicker.statusLevel <= 100) {
                    activeServiceFlicker.direct = true;
                }
                activeServiceFlicker.statusLevel -= 10;
            }
        }
        if (activeServiceProjectFlicker.isvalid && serviceSetActive()) {
            for (int ListIndex = 0; ListIndex < activeServiceProjectFlicker.Item.size(); ListIndex++) {
                QStandardItem *item = activeServiceProjectFlicker.Item.at(ListIndex);
                if (item) {
                    item->setBackground(QBrush(((activeServiceProjectFlicker.statusLevel / 10 + 155) << 8) + 255) );
                }
            }
            if (activeServiceProjectFlicker.direct) {
                if (activeServiceProjectFlicker.statusLevel >= 900) {
                    activeServiceProjectFlicker.direct = false;
                }
                activeServiceProjectFlicker.statusLevel += 10;
            }
            else {
                if (activeServiceProjectFlicker.statusLevel <= 100) {
                    activeServiceProjectFlicker.direct = true;
                }
                activeServiceProjectFlicker.statusLevel -= 10;
            }
        }
    });

    createAdbProcess();
}

void UDS::readUserActionCache()
{
    QDir dir(serviceBackCacheDir);
    if (!dir.exists()) {
        dir.mkdir(serviceBackCacheDir);
    }
    readServiceSet(serviceBackCacheDir + "/serviceSetCache.json");

    QSettings settings(serviceBackCacheDir + "udsOperationSettings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("utf-8"));

    cycleNumberEdit->setText(QString("%1").arg(settings.value("serviceTest/cycleNumber").toInt()));
    serviceTestResultCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/serviceTestResultCheckBox").toInt());
    UDSDescolumnCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/UDSDescolumnCheckBox").toInt());
    UDSResponseParseCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/UDSResponseParseCheckBox").toInt());
    TestProjectNameEdit->setCurrentText(settings.value("serviceTest/TestProjectNameEdit").toString());
    SendBtnCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/SendBtnCheckBox").toInt());
    ConfigBtnCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/ConfigBtnCheckBox").toInt());
    UpBtnCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/UpBtnCheckBox").toInt());
    DownBtnCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/DownBtnCheckBox").toInt());
    DelBtnCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/DelBtnCheckBox").toInt());
    doipDiagCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/doipDiagCheckBox").toInt());
    canDiagCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/canDiagCheckBox").toInt());
    canRequestAddr->setText(settings.value("serviceTest/canRequestAddr").toString());
    canResponseFuncAddr->setText(settings.value("serviceTest/canResponseFuncAddr").toString());
    canResponsePhysAddr->setText(settings.value("serviceTest/canResponsePhysAddr").toString());
    doipResponseFuncAddr->setText(settings.value("serviceTest/doipResponseFuncAddr").toString());
    doipResponsePhysAddr->setText(settings.value("serviceTest/doipResponsePhysAddr").toString());
    cycle3eEdit->setText(settings.value("serviceTest/cycle3eEdit").toString());
    taAdress3eEdit->setText(settings.value("serviceTest/taAdress3eEdit").toString());
    saAdress3eEdit->setText(settings.value("serviceTest/saAdress3eEdit").toString());
    highPerformanceCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/highPerformanceCheckBox").toInt());
    failureAbortCheckBox->setCheckState((Qt::CheckState)settings.value("serviceTest/failureAbortCheckBox").toInt());
}

void UDS::writeUserActionCache()
{
    QSettings settings(serviceBackCacheDir + "udsOperationSettings.ini", QSettings::IniFormat);

    settings.setIniCodec(QTextCodec::codecForName("utf-8"));
    settings.setValue("serviceTest/cycleNumber", cycleNumberEdit->text().toInt());
    settings.setValue("serviceTest/serviceTestResultCheckBox", serviceTestResultCheckBox->checkState());
    settings.setValue("serviceTest/UDSDescolumnCheckBox", UDSDescolumnCheckBox->checkState());
    settings.setValue("serviceTest/UDSResponseParseCheckBox", UDSResponseParseCheckBox->checkState());
    settings.setValue("serviceTest/TestProjectNameEdit", TestProjectNameEdit->currentText());
    settings.setValue("serviceTest/SendBtnCheckBox", SendBtnCheckBox->checkState());
    settings.setValue("serviceTest/ConfigBtnCheckBox", ConfigBtnCheckBox->checkState());
    settings.setValue("serviceTest/UpBtnCheckBox", UpBtnCheckBox->checkState());
    settings.setValue("serviceTest/DownBtnCheckBox", DownBtnCheckBox->checkState());
    settings.setValue("serviceTest/DelBtnCheckBox", DelBtnCheckBox->checkState());
    settings.setValue("serviceTest/doipDiagCheckBox", doipDiagCheckBox->checkState());
    settings.setValue("serviceTest/canDiagCheckBox", canDiagCheckBox->checkState());
    settings.setValue("serviceTest/canRequestAddr", canRequestAddr->text());
    settings.setValue("serviceTest/canResponseFuncAddr", canResponseFuncAddr->text());
    settings.setValue("serviceTest/canResponsePhysAddr", canResponsePhysAddr->text());
    settings.setValue("serviceTest/doipResponseFuncAddr", doipResponseFuncAddr->text());
    settings.setValue("serviceTest/doipResponsePhysAddr", doipResponsePhysAddr->text());
    settings.setValue("serviceTest/cycle3eEdit", cycle3eEdit->text());
    settings.setValue("serviceTest/taAdress3eEdit", taAdress3eEdit->text());
    settings.setValue("serviceTest/saAdress3eEdit", saAdress3eEdit->text());
    settings.setValue("serviceTest/highPerformanceCheckBox", highPerformanceCheckBox->checkState());
    settings.setValue("serviceTest/failureAbortCheckBox", failureAbortCheckBox->checkState());
    settings.sync();
}

QWidget *UDS::mainWidget()
{
    ProjectListWidget = serviceProjectListWidget();
    ProjectListWidget->hide();

    QString progressStyle = "QProgressBar {"
                       "border: 1px solid #2E8B57;"
                       "border-radius: 2px;"
                       "text-align: center;"
                       "background-color: #fff;"
                   "}"
                   "QProgressBar::chunk {"
                       "background-color: #2E8B57;"
                       "width: 10px;"
                       "margin: 0.5px;"
                       "border-radius: 1px;"
                   "}";
    QWidget *udsMainWidget = new QWidget();
    QVBoxLayout *udsMainLayout = new QVBoxLayout();
    udsMainWidget->setLayout(udsMainLayout);

    QVBoxLayout *topLayout = new QVBoxLayout();
    udsMainLayout->addLayout(topLayout);

    promptActionLabel = new QLabel();
    promptActionLabel->setOpenExternalLinks(true);
    topLayout->addWidget(promptActionLabel);
    promptActionLabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *progressLayout = new QHBoxLayout();
    topLayout->addLayout(progressLayout);
    progressLayout->addStretch();
    promptActionProgress = new QProgressBar();
    progressLayout->addWidget(promptActionProgress);
    progressLayout->addStretch();
    promptActionProgress->setStyleSheet("QProgressBar {"
                                        "border: 1px solid #00BFFF;"
                                        "border-radius: 2px;"
                                        "text-align: center;"
                                        "background-color: #fff;"
                                    "}"
                                    "QProgressBar::chunk {"
                                        "background-color: #00BFFF;"//这是方块颜色，#FFC1C1为粉色
                                        "width: 5px;"
                                        "margin: 0.5px;"
                                        "border-radius: 1px;"
                                    "}");
    promptActionProgress->setMaximumHeight(10);
    promptActionProgress->setMinimum(0);
    promptActionProgress->setMaximumWidth(400);
    promptActionProgress->setOrientation(Qt::Horizontal);
    promptActionProgress->hide();

    udstabWidget = new QTabWidget();
    udstabWidget->setStyleSheet("QTabBar::tab:selected {\
                               color: #FFFFFF;\
                               background-color: #33A1C9;\
                              } \
                              QTabBar::tab:!selected {\
                               color: #000000;\
                               background-color: #FFFFFF;\
                              }"\
                              "QTabWidget::pane {\
                               border:1px solid #33A1C9;\
                               background: transparent;\
                               margin-top: 0px;\
                              }");
    udstabWidget->setMaximumHeight(130);
    udsMainLayout->addWidget(udstabWidget);
    udstabWidget->addTab(operationConfigWidget(), "操作选项");
    udstabWidget->setTabIcon(udstabWidget->count() - 1, QIcon(":/icon/operation.png"));
    // udsMainLayout->addWidget(channelSelectWidget());
    udstabWidget->addTab(channelSelectWidget(), "通道选择");
    udstabWidget->setTabIcon(udstabWidget->count() - 1, QIcon(":/icon/channel.png"));
    // udsMainLayout->addWidget(operationConfigWidget());
    // udsMainLayout->addWidget(userSelectWidget());
    udstabWidget->addTab(serviceGeneralConfigWidget(), "UDS通用配置");
    udstabWidget->setTabIcon(udstabWidget->count() - 1, QIcon(":/icon/reset.png"));
    udstabWidget->addTab(uiConfigWidget(), "界面配置");
    udstabWidget->setTabIcon(udstabWidget->count() - 1, QIcon(":/icon/interface.png"));

    progressWidget = new QWidget();
    progressWidget->setContentsMargins(0, 0, 0, 0);
    progressLayout = new QHBoxLayout();
    progressWidget->setLayout(progressLayout);
    udsMainLayout->addWidget(progressWidget);

    serviceTestProgress = new QProgressBar();
    serviceTestProgress->setMaximumHeight(15);
    progressWidget->setHidden(true);
    serviceTestProgress->setMinimum(0);
    serviceTestProgress->setOrientation(Qt::Horizontal);
    progressLayout->addWidget(serviceTestProgress);
    serviceTestProgress->setStyleSheet(progressStyle);

    QLabel *upTransferSpeedIconLable = new QLabel();
    upTransferSpeedIconLable->setMaximumWidth(20);
    upTransferSpeedIconLable->setPixmap(QPixmap(":/icon/up_transfer.png").scaled(15, 15, Qt::KeepAspectRatio));
    progressLayout->addWidget(upTransferSpeedIconLable);
    upTransferSpeedLable = new QLabel();
    upTransferSpeedLable->setText("0 KB/s");
    progressLayout->addWidget(upTransferSpeedLable);

    QLabel *downTransferSpeedIconLable = new QLabel();
    downTransferSpeedIconLable->setMaximumWidth(20);
    downTransferSpeedIconLable->setPixmap(QPixmap(":/icon/down_transfer.png").scaled(15, 15, Qt::KeepAspectRatio));
    progressLayout->addWidget(downTransferSpeedIconLable);
    downTransferSpeedLable = new QLabel();
    downTransferSpeedLable->setText("0 KB/s");
    progressLayout->addWidget(downTransferSpeedLable);

    QHBoxLayout *statisLayout = new QHBoxLayout();
    serviceItemStatisLabel.insert(serviceItemAll, new QLabel("总共:0"));
    serviceItemStatisLabel.insert(serviceItemNotTest, new QLabel("未测试:0"));
    serviceItemStatisLabel.insert(serviceItemTestPass, new QLabel("通过:0"));
    serviceItemStatisLabel.insert(serviceItemTestFail, new QLabel("失败:0"));
    serviceItemStatisLabel.insert(serviceItemTestWarn, new QLabel("警告:0"));
    statisLayout->addWidget(serviceItemStatisLabel[serviceItemAll]);
    statisLayout->addWidget(serviceItemStatisLabel[serviceItemNotTest]);
    statisLayout->addWidget(serviceItemStatisLabel[serviceItemTestPass]);
    statisLayout->addWidget(serviceItemStatisLabel[serviceItemTestFail]);
    statisLayout->addWidget(serviceItemStatisLabel[serviceItemTestWarn]);
    serviceItemStatisLabel[serviceItemAll]->setFixedHeight(15);
    serviceItemStatisLabel[serviceItemNotTest]->setFixedHeight(15);
    serviceItemStatisLabel[serviceItemTestPass]->setFixedHeight(15);
    serviceItemStatisLabel[serviceItemTestFail]->setFixedHeight(15);
    serviceItemStatisLabel[serviceItemTestWarn]->setFixedHeight(15);
    serviceItemStatisLabel[serviceItemAll]->setStyleSheet(QString("QLabel{background-color: %1; height: 25px;}").arg(serviceItemStateColor[serviceItemAll]));
    serviceItemStatisLabel[serviceItemNotTest]->setStyleSheet(QString("QLabel{background-color: %1;height: 25px;}").arg(serviceItemStateColor[serviceItemNotTest]));
    serviceItemStatisLabel[serviceItemTestPass]->setStyleSheet(QString("QLabel{background-color: %1;height: 25px;}").arg(serviceItemStateColor[serviceItemTestPass]));
    serviceItemStatisLabel[serviceItemTestFail]->setStyleSheet(QString("QLabel{background-color: %1;height: 25px;}").arg(serviceItemStateColor[serviceItemTestFail]));
    serviceItemStatisLabel[serviceItemTestWarn]->setStyleSheet(QString("QLabel{background-color: %1;height: 25px;}").arg(serviceItemStateColor[serviceItemTestWarn]));
    udsMainLayout->addLayout(statisLayout);
#ifdef __HAVE_CHARTS__
    chart = new QChart();
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setContentsMargins(0, 0, 0, 0);  //设置外边界全部为0
    chart->setMargins(QMargins(0, 0, 0, 0)); //设置内边界全部为0
    chart->setBackgroundRoundness(0);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    axisY = new QBarCategoryAxis();
    axisY->append("结果统计");
    chart->addAxis(axisY, Qt::AlignLeft);
    axisX = new QValueAxis();
    chart->addAxis(axisX, Qt::AlignBottom);
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setFixedHeight(110);
    chartView->setContentsMargins(0, 0, 0, 0);  //设置外边界全部为0
    chartView->hide();
    udsMainLayout->addWidget(chartView);
#endif /* __HAVE_CHARTS__  */
    //serviceProjectLayout->addStretch();

    QHBoxLayout *testTableLayout = new QHBoxLayout();
    udsMainLayout->addLayout(testTableLayout);
    testTableLayout->addWidget(ProjectListWidget);

    serviceSetTabHeaderTitle.insert(udsViewItemEnable, "使能");
    serviceSetTabHeaderTitle.insert(udsViewRequestColumn, "请求(hex)");
    serviceSetTabHeaderTitle.insert(udsViewSupressColumn, "抑制");
    serviceSetTabHeaderTitle.insert(udsViewTimeoutColumn, "超时(ms)");
    serviceSetTabHeaderTitle.insert(udsViewDelayColumn, "延时(ms)");
    serviceSetTabHeaderTitle.insert(udsViewElapsedTimeColumn, "耗时(ms)");
    serviceSetTabHeaderTitle.insert(udsViewResponseColumn, "响应(hex)");
    serviceSetTabHeaderTitle.insert(udsViewExpectResponseColumn, "预期响应(hex)");
    serviceSetTabHeaderTitle.insert(udsViewRemarkColumn, "备注");
    serviceSetTabHeaderTitle.insert(udsViewSendBtnColumn, "发送");
    serviceSetTabHeaderTitle.insert(udsViewConfigBtnColumn, "配置");
    serviceSetTabHeaderTitle.insert(udsViewUpBtnColumn, "上移");
    serviceSetTabHeaderTitle.insert(udsViewDownBtnColumn, "下移");
    serviceSetTabHeaderTitle.insert(udsViewDelBtnColumn, "删除");
    serviceSetTabHeaderTitle.insert(udsViewResultColumn, "结果");

    serviceSetModel = new QStandardItemModel();
    serviceSetModel->setColumnCount(serviceSetTabHeaderTitle.length());
    serviceSetModel->setHorizontalHeaderLabels(serviceSetTabHeaderTitle);
    serviceSetView = new QTableView();
    serviceSetView->setModel(serviceSetModel);
    QItemSelectionModel *theSelection = new QItemSelectionModel(serviceSetModel) ; //选择模型
    connect(theSelection, &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current, const QModelIndex &previous){
        return ;
        Q_UNUSED(previous);
        if (current.isValid()){
            if (udsViewResponseColumn == current.column() /* || udsViewExpectResponseColumn == current.column() */) {
                int column[] = {udsViewResponseColumn/*, udsViewExpectResponseColumn*/};
                int columnHeight = 30;
                serviceSetView->verticalHeader()->setMinimumSectionSize(30);
                for (unsigned int cc = 0; cc < sizeof(column) / sizeof(column[0]); cc++) {
                    QStandardItem *item = serviceSetModel->item(current.row(), column[cc]);
                    if (item && item->text().count('\n')) {
                        QString parsedata = item->toolTip();
                        item->setToolTip(item->text());
                        item->setText(parsedata);
                    } else {
                        QByteArray bytedata = QByteArray::fromHex(item->text().remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                        QString parseStr = udsParse(bytedata);
                        item->setText(parseStr);
                        item->setToolTip(QString(bytedata.toHex(' ')));
                        columnHeight = fontHeight *  parseStr.count('\n') > columnHeight ? fontHeight *  parseStr.count('\n'): columnHeight;
                    }
                }
                serviceSetView->verticalHeader()->resizeSection(current.row(), columnHeight);
            }
        }
    });
    serviceSetView-> setSelectionModel(theSelection) ;
    serviceSetView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    serviceSetView->setSelectionBehavior(QAbstractItemView::SelectItems);
    serviceSetView->setContextMenuPolicy(Qt::CustomContextMenu);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewItemEnable, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewSupressColumn, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewTimeoutColumn, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewDelayColumn, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewConfigBtnColumn, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewSendBtnColumn, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewDelBtnColumn, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewUpBtnColumn, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewDownBtnColumn, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setSectionResizeMode(udsViewElapsedTimeColumn, QHeaderView::ResizeToContents);
    serviceSetView->horizontalHeader()->setStretchLastSection(true);
    serviceSetView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    serviceSetView->setAlternatingRowColors(true);
    serviceSetView->setColumnHidden(udsViewResultColumn, true);
    testTableLayout->addWidget(serviceSetView);

    serviceSetMenu = new QMenu();
    QAction *serviceSetDelAction = new QAction("删除此行");
    serviceSetDelAction->setIcon(QIcon(":/icon/delete.png"));
    serviceSetMenu->addAction(serviceSetDelAction);

    QAction *serviceSetChangeAction = new QAction("修改配置");
    serviceSetChangeAction->setIcon(QIcon(":/icon/config1.png"));
    serviceSetMenu->addAction(serviceSetChangeAction);

    QAction *serviceSetUpAction = new QAction("上移一行");
    serviceSetUpAction->setIcon(QIcon(":/icon/up.png"));
    serviceSetMenu->addAction(serviceSetUpAction);

    QAction *serviceSetDownAction = new QAction("下移一行");
    serviceSetDownAction->setIcon(QIcon(":/icon/down.png"));
    serviceSetMenu->addAction(serviceSetDownAction);

    QAction *serviceSetMoveAction = new QAction("移动至行");
    serviceSetMoveAction->setIcon(QIcon(":/icon/move.png"));
    serviceSetMenu->addAction(serviceSetMoveAction);

    QDialog *serviceSetMoveDiag = new QDialog();
    serviceSetMoveDiag->setAttribute(Qt::WA_QuitOnClose, false);
    serviceSetMoveDiag->setWindowTitle("移动至行");
    QHBoxLayout *serviceSetMoveDiagLayout = new QHBoxLayout();
    serviceSetMoveDiag->setLayout(serviceSetMoveDiagLayout);
    QLabel *serviceSetMoveHint = new QLabel("移动到");
    serviceSetMoveDiagLayout->addWidget(serviceSetMoveHint);
    QLineEdit *serviceSetMoveDiagEdit = new QLineEdit();
    serviceSetMoveDiagEdit->setMaximumWidth(40);
    serviceSetMoveDiagEdit->setValidator(new QIntValidator(0, 100000, serviceSetMoveDiagEdit));
    serviceSetMoveDiagLayout->addWidget(serviceSetMoveDiagEdit);
    serviceSetMoveDiagLayout->addWidget(new QLabel("行"));
    QPushButton *serviceSetMoveDiagConfirmBtn = new QPushButton("确定");
    serviceSetMoveDiagLayout->addWidget(serviceSetMoveDiagConfirmBtn);
    connect(serviceSetMoveDiagConfirmBtn, &QPushButton::clicked, this, \
            [this, serviceSetMoveDiag, serviceSetMoveDiagEdit] {
        serviceSetMoveDiag->hide();
        int newRow = serviceSetMoveDiagEdit->text().toInt();
        serviceSetMoveItem(serviceSetView->selectionModel()->currentIndex().row(), newRow - 1);
    });

    connect(serviceSetDelAction, &QAction::triggered, this, [this]{
        switch (QMessageBox::information(0, tr("删除"), tr("确认删除?"), tr("是"), tr("否"), 0, 1 )) {
          case 0:break;
          case 1:default: return ; break;
        }
        serviceSetDelItem(serviceSetView->selectionModel()->currentIndex().row());
    });

    connect(serviceSetChangeAction, &QAction::triggered, this, [this]{
        serviceSetChangeItem(serviceSetView->selectionModel()->currentIndex().row());
    });

    connect(serviceSetUpAction, &QAction::triggered, this, [this]{
        serviceSetMoveItem(serviceSetView->selectionModel()->currentIndex().row(),\
                             serviceSetView->selectionModel()->currentIndex().row() - 1);
    });

    connect(serviceSetDownAction, &QAction::triggered, this, [this]{
        serviceSetMoveItem(serviceSetView->selectionModel()->currentIndex().row(),\
                             serviceSetView->selectionModel()->currentIndex().row() + 1);
    });

    connect(serviceSetMoveAction, &QAction::triggered, this, [this, serviceSetMoveDiag, serviceSetMoveHint]{
        serviceSetMoveDiag->hide();
        serviceSetMoveHint->setText(QString("第%1移动到").arg(serviceSetView->selectionModel()->currentIndex().row() + 1));
        serviceSetMoveDiag->show();
    });

    connect(serviceSetView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    udsViewSetColumnHidden(true);

    serviceSetBackTimer = new QTimer();
    serviceSetBackTimer->setInterval(1000);
    connect(serviceSetBackTimer, &QTimer::timeout, this, [this](){
        writeServiceSetCache();
        serviceSetBackTimer->stop();
    });
    readUserActionCache();

    return udsMainWidget;
}

QWidget *UDS::userSelectWidget()
{
    QWidget *mainWidget = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainWidget->setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    return mainWidget;
}

QWidget *UDS::channelSelectWidget()
{
    QWidget *mainWidget = new QWidget();

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainWidget->setLayout(mainLayout);
    mainWidget->setMaximumHeight(85);

    QHBoxLayout *channelSelectLayout = new QHBoxLayout();
    mainLayout->addLayout(channelSelectLayout);

    QHBoxLayout *doipInfoLayout = new QHBoxLayout();
    doipInfoLayout->setContentsMargins(0, 0, 0, 0);
    QStackedWidget *stackedWidget = new QStackedWidget();
    stackedWidget->setMaximumHeight(100);
    mainLayout->addWidget(stackedWidget);
    doipChannelConfigWidget = new QWidget();
    doipChannelConfigWidget->setLayout(doipInfoLayout);
    // mainLayout->addWidget(doipChannelConfigWidget);
    stackedWidget->addWidget(doipChannelConfigWidget);
    doipDiagCheckBox = new QCheckBox("UDSonIP");
    channelSelectLayout->addWidget(doipDiagCheckBox);
    doipInfoLayout->addWidget(new QLabel("DOIP通道:"));
    doipServerListBox = new QComboBox();
    doipServerListBox->setMinimumWidth(160);
    doipServerListBox->addItem("");
    doipInfoLayout->addWidget(doipServerListBox);
    doipResponsePhysAddr = new QLineEdit();
    doipResponsePhysAddr->setMaximumWidth(60);
    doipResponsePhysAddr->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{4}$"), this));
    doipInfoLayout->addWidget(new QLabel("物理地址:"));
    doipInfoLayout->addWidget(doipResponsePhysAddr);
    doipResponseFuncAddr = new QLineEdit();
    doipResponseFuncAddr->setMaximumWidth(60);
    doipResponseFuncAddr->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{4}$"), this));
    doipInfoLayout->addWidget(new QLabel("功能地址:"));
    doipInfoLayout->addWidget(doipResponseFuncAddr);
    doipInfoLayout->addStretch();
    connect(doipServerListBox, &QComboBox::currentTextChanged, this, [this](const QString &text){
        if (text.size() != 0) {
            adbDiagCheckBox->setCheckState(Qt::Unchecked);
        }
    });

    QHBoxLayout *candiagLayout = new QHBoxLayout();
    candiagLayout->setContentsMargins(0, 0, 0, 0);
    canChannelConfigWidget = new QWidget();
    canChannelConfigWidget->setLayout(candiagLayout);
    // mainLayout->addWidget(canChannelConfigWidget);
    stackedWidget->addWidget(canChannelConfigWidget);

    canDiagCheckBox = new QCheckBox("UDSonCAN");
    channelSelectLayout->addWidget(canDiagCheckBox);
    canDiagCheckBox->setCheckState(Qt::Unchecked);
    candiagLayout->addWidget(can->deviceManageWidget());
    canRequestAddr = new QLineEdit();
    canRequestAddr->setMaximumWidth(80);
    canRequestAddr->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{8}$"), this));
    candiagLayout->addWidget(new QLabel("请求地址:"));
    candiagLayout->addWidget(canRequestAddr);
    canResponsePhysAddr = new QLineEdit();
    canResponsePhysAddr->setMaximumWidth(80);
    canResponsePhysAddr->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{8}$"), this));
    candiagLayout->addWidget(new QLabel("物理地址:"));
    candiagLayout->addWidget(canResponsePhysAddr);
    canResponseFuncAddr = new QLineEdit();
    canResponseFuncAddr->setMaximumWidth(80);
    canResponseFuncAddr->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{8}$"), this));
    candiagLayout->addWidget(new QLabel("功能地址:"));
    candiagLayout->addWidget(canResponseFuncAddr);
    candiagLayout->addStretch();

    QHBoxLayout *adbDiagLayout = new QHBoxLayout();
    adbDiagLayout->setContentsMargins(0, 0, 0, 0);
    adbChannelConfigWidget = new QWidget();
    adbChannelConfigWidget->setLayout(adbDiagLayout);
    //mainLayout->addWidget(adbChannelConfigWidget);
    stackedWidget->addWidget(adbChannelConfigWidget);
    adbDiagCheckBox = new QCheckBox("UDSonADB");
    channelSelectLayout->addWidget(adbDiagCheckBox);
    adbDiagCheckBox->setCheckState(Qt::Unchecked);

    adbStateLabel = new QLabel("未登录");
    adbDiagLayout->addWidget(adbStateLabel);
    adbDiagLayout->addStretch();

    adbLoginHintEdit = new QComboBox();
    adbLoginHintEdit->setEditable(true);
    adbDiagLayout->addWidget(new QLabel("用户名提示符:"));
    adbDiagLayout->addWidget(adbLoginHintEdit);
    adbLoginHintEdit->addItem("login");
    adbDiagLayout->addStretch();

    adbLoginNameEdit = new QComboBox();
    adbLoginNameEdit->setEditable(true);
    adbDiagLayout->addWidget(new QLabel("用户名:"));
    adbDiagLayout->addWidget(adbLoginNameEdit);
    adbLoginNameEdit->lineEdit()->setEchoMode(QLineEdit::Password);
    adbLoginNameEdit->lineEdit()->setText("root");
    adbLoginNameEdit->setMinimumWidth(100);
    adbDiagLayout->addStretch();

    adbPasswordHintEdit = new QComboBox();
    adbPasswordHintEdit->setEditable(true);
    adbDiagLayout->addWidget(new QLabel("密码提示符:"));
    adbDiagLayout->addWidget(adbPasswordHintEdit);
    adbPasswordHintEdit->addItem("Password");
    adbDiagLayout->addStretch();

    adbPasswordEdit = new QComboBox();
    adbPasswordEdit->setEditable(true);
    adbDiagLayout->addWidget(new QLabel("密码:"));
    adbDiagLayout->addWidget(adbPasswordEdit);
    adbPasswordEdit->lineEdit()->setEchoMode(QLineEdit::Password);
    adbPasswordEdit->lineEdit()->setText("qwer1234");
    adbPasswordEdit->setMinimumWidth(100);
    adbDiagLayout->addStretch();

    connect(adbDiagCheckBox, &QCheckBox::stateChanged, this, [this, stackedWidget](int state){
        if (serviceActive()) {
            promptForAction(QString("<font color = red>测试中无法操作</font>"));
            return ;
        }

        if (Qt::Checked == state) {
            doipServerListBox->setCurrentText("");
            doipServerListBox->setEnabled(false);
            adbListenTimer->start();
            doipDiagCheckBox->setCheckState(Qt::Unchecked);
            canDiagCheckBox->setCheckState(Qt::Unchecked);
            IPDiagPcapCheckBox->setEnabled(false);
            stackedWidget->setCurrentWidget(adbChannelConfigWidget);
            udstabWidget->setTabText(1, "ADB通道");
        } else {
            doipServerListBox->setEnabled(true);
            adbListenTimer->stop();
        }
    });

    connect(canDiagCheckBox, &QCheckBox::stateChanged, this, [this, stackedWidget](int state){
        if (serviceActive()) {
            promptForAction(QString("<font color = red>测试中无法操作</font>"));
            return ;
        }

        if (Qt::Checked == state) {
            doipDiagCheckBox->setCheckState(Qt::Unchecked);
            adbDiagCheckBox->setCheckState(Qt::Unchecked);
            IPDiagPcapCheckBox->setEnabled(false);
            stackedWidget->setCurrentWidget(canChannelConfigWidget);
            udstabWidget->setTabText(1, "CAN通道");
        }
    });

    connect(doipDiagCheckBox, &QCheckBox::stateChanged, this, [this, stackedWidget](int state){
        if (serviceActive()) {
            promptForAction(QString("<font color = red>测试中无法操作</font>"));
            return ;
        }

        if (Qt::Checked == state) {
            canDiagCheckBox->setCheckState(Qt::Unchecked);
            adbDiagCheckBox->setCheckState(Qt::Unchecked);
            IPDiagPcapCheckBox->setEnabled(true);
            stackedWidget->setCurrentWidget(doipChannelConfigWidget);
            udstabWidget->setTabText(1, "DOIP通道");
        }
    });
    channelSelectLayout->addStretch();
    mainLayout->addStretch();

    return mainWidget;
}

QWidget *UDS::operationConfigWidget()
{
    QWidget *mainWidget = new QWidget();

    mainWidget->setStyleSheet(btnCommonStyle);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainWidget->setLayout(mainLayout);

    QHBoxLayout *serviceConfigLayout = new QHBoxLayout();
    mainLayout->addLayout(serviceConfigLayout);

    addServiceBtn = new QPushButton("服务配置");
    addServiceBtn->setIcon(QIcon(":/icon/config1.png"));
    connect(addServiceBtn, &QPushButton::clicked, this, [this]{
        if (serviceActive()) {
            promptForAction(QString("<font color = red>测试中无法%1</font>").arg(addServiceBtn->text()));
            return ;
        }
        modifyRowIndex = -1;
        udsRequestConfigBtn->setText("诊断服务添加至列表");
        serviceSetWidget->show();
        udsViewSetColumnHidden(false);
    });

    addDidDescBtn = new QPushButton("DID描述");
    addDidDescBtn->setIcon(QIcon(":/icon/addList.png"));
    connect(addDidDescBtn, &QPushButton::clicked, this, []{
        qDebug() << "功能未实现";
        QMessageBox::warning(nullptr, tr("提示"), tr("功能未实现"));
    });

    addDtcDescBtn = new QPushButton("DTC描述");
    addDtcDescBtn->setIcon(QIcon(":/icon/addList.png"));
    connect(addDtcDescBtn, &QPushButton::clicked, this, []{
        qDebug() << "功能未实现";
        QMessageBox::warning(nullptr, tr("提示"), tr("功能未实现"));
    });

    QHBoxLayout *udsProjectLayout = new QHBoxLayout();
    mainLayout->addLayout(udsProjectLayout);

    udsProjectLayout->addWidget(addDidDescBtn);
    udsProjectLayout->addWidget(addDtcDescBtn);
    udsProjectLayout->addWidget(addServiceBtn);

    QDir dir(serviceSetProjectDir);
    if (!dir.exists()) {
        dir.mkdir(serviceSetProjectDir);
    }
    udsProjectLayout->addWidget(new QLabel("服务集合:"));
    TestProjectNameEdit = new QComboBox();
    TestProjectNameEdit->setEditable(true);
    TestProjectNameEdit->setMinimumWidth(250);
    QFileInfoList list = dir.entryInfoList();
    dir.setNameFilters(QStringList() << "*.json");
    TestProjectNameEdit->clear();
    foreach(QFileInfo fileinfo, list) {
        if (fileinfo.fileName().contains("json")) {
            TestProjectNameEdit->addItem(fileinfo.fileName());
            projectNameComBox->addItem(fileinfo.fileName());
            qDebug() << fileinfo.fileName();
        }
    }
    udsProjectLayout->addWidget(TestProjectNameEdit);
    saveTestProjectBtn = new QPushButton("保存方案");
    saveTestProjectBtn->setIcon(QIcon(":/icon/save.png"));
    udsProjectLayout->addWidget(saveTestProjectBtn);
    connect(saveTestProjectBtn, &QPushButton::clicked, this, [this]{
        if (serviceActive()) {
            promptForAction(QString("<font color = red>测试中无法%1</font>").arg(saveTestProjectBtn->text()));
            return ;
        }
        if (serviceSetModel->rowCount() == 0) {
            promptForAction(QString("<font color = red>%1表格中无数据项</font>").arg(saveTestProjectBtn->text()));
            return ;
        }
        QString fileName = TestProjectNameEdit->lineEdit()->text();
        if (fileName.size() == 0) {
            promptForAction(QString("<font color = red>%1失败条件文件名长度不能为0</font>").arg(saveTestProjectBtn->text()));
            return ;
        }
        qDebug() << fileName;
        QFileInfo fileinfo(serviceSetProjectDir + "/" + fileName);
        if (fileinfo.isFile()) {
            switch (QMessageBox::information(0, tr("覆盖文件"), tr("是否覆盖同名文件?"), tr("是"), tr("否"), 0, 1 )) {
              case 0: break;
              case 1: default: return ; break;
            }
            fileName =  serviceSetProjectDir + "/" + fileName;
        }
        else {
            if (fileName.contains("json")) {
                promptForAction(QString("<font color = red>%1失败条件不满足(文件名不能包含“json”字符串)</font>").arg(saveTestProjectBtn->text()));
                return ;
            }
            fileName =  serviceSetProjectDir + "/" + fileName + ".json";
        }
        saveCurrServiceSet(fileName);
        TestProjectNameEdit->addItem(fileinfo.fileName());
        TestProjectNameEdit->setCurrentText(fileinfo.fileName());
        projectNameComBox->addItem(fileinfo.fileName());
        projectNameComBox->setCurrentText(fileinfo.fileName());
        promptForAction(QString("<a style='color: rgb(30, 144, 255);' href=\"file:///%1\">保存成功:%2</a>").arg(serviceSetProjectDir).arg(fileName));
    });
    loadTestProjectBtn = new QPushButton("加载方案");
    loadTestProjectBtn->setIcon(QIcon(":/icon/download.png"));
    udsProjectLayout->addWidget(loadTestProjectBtn);
    connect(loadTestProjectBtn, &QPushButton::clicked, this, [this]{
        if (serviceActive()) {
            promptForAction(QString("<font color = red>测试中无法%1</font>").arg(loadTestProjectBtn->text()));
            return ;
        }

        switch (QMessageBox::information(0, tr("加载方案"), tr("确认加载?"), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default: return ; break;
        }

        readServiceSet(serviceSetProjectDir + "/" + TestProjectNameEdit->lineEdit()->text());
        promptForAction(QString("<font color = green>%1成功</font>").arg(loadTestProjectBtn->text()));
    });

    delTestProjectBtn = new QPushButton("删除方案");
    delTestProjectBtn->setIcon(QIcon(":/icon/delete.png"));
    udsProjectLayout->addWidget(delTestProjectBtn);
    connect(delTestProjectBtn, &QPushButton::clicked, this, [this]{
        if (serviceActive()) {
            promptForAction(QString("<font color = red>测试中无法%1</font>").arg(delTestProjectBtn->text()));
            return ;
        }

        switch (QMessageBox::information(0, tr("删除"), tr("确认删除?"), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default: return ; break;
        }
        QString fileName = TestProjectNameEdit->lineEdit()->text();
        if (fileName.size() != 0) {
            fileName =  serviceSetProjectDir + "/" + fileName;
            QFile file(fileName);
            file.remove();
            TestProjectNameEdit->removeItem(TestProjectNameEdit->currentIndex());
            TestProjectNameEdit->lineEdit()->clear();
            projectNameComBox->removeItem(TestProjectNameEdit->currentIndex());
        }
        promptForAction(QString("<font color = green>%1成功</font>").arg(delTestProjectBtn->text()));
    });
    QHBoxLayout *udsOpInfoLayout = new QHBoxLayout();
    mainLayout->addLayout(udsOpInfoLayout);
    udsOpInfoLayout->addWidget(new QLabel("循环测试:"));
    cycleNumberEdit = new QLineEdit();
    cycleNumberEdit->setValidator(new QIntValidator(0, 100000, cycleNumberEdit));
    cycleNumberEdit->setMaximumWidth(60);
    udsOpInfoLayout->addWidget(cycleNumberEdit);
    //udsOpInfoLayout->addStretch();

    udsOpInfoLayout->addWidget(new QLabel("单耗时(ms):"));
    serviceElapsedTimeLCDNumber = new QLCDNumber();
    serviceElapsedTimeLCDNumber->setDigitCount(7);
    serviceElapsedTimeLCDNumber->setSegmentStyle(QLCDNumber::Flat);
    udsOpInfoLayout->addWidget(serviceElapsedTimeLCDNumber);
    //udsOpInfoLayout->addStretch();

    udsOpInfoLayout->addWidget(new QLabel("总耗时(ms):"));
    allServiceElapsedTimeLCDNumber = new QLCDNumber();
    allServiceElapsedTimeLCDNumber->setDigitCount(9);
    allServiceElapsedTimeLCDNumber->setSegmentStyle(QLCDNumber::Flat);
    udsOpInfoLayout->addWidget(allServiceElapsedTimeLCDNumber);

    testResultCheckBox = new QCheckBox("保存结果");
    udsOpInfoLayout->addWidget(testResultCheckBox);
    connect(testResultCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (Qt::Checked == state) {
            UDSResponseParseCheckBox->setCheckState(Qt::Unchecked);
            UDSResponseParseCheckBox->setEnabled(false);
            cycleNumberEdit->setEnabled(false);
            loadTestProjectBtn->setEnabled(false);
            addServiceBtn->setEnabled(false);
            serviceSetclearBtn->setEnabled(false);
            QString filename = generateTestResultFileName();
            quint32 totalRow = (serviceSetModel->rowCount() + 1) * (cycleNumberEdit->text().toUInt()) + 1;
            if (totalRow > 20000) {
                promptForAction(QString("<a style='color: rgb(255, 0, 0);' href=\"file:///%1\">保存文件:%2，预计行数%3行, 可能造成界面卡顿</a>").\
                                arg(QFileInfo(filename).path()).arg(QFileInfo(filename).fileName()).arg(totalRow));
            }
            else {
                promptForAction(QString("<a style='color: rgb(30, 144, 255);' href=\"file:///%1\">保存文件:%2，预计行数%3行</a>").\
                                arg(QFileInfo(filename).path()).arg(QFileInfo(filename).fileName()).arg(totalRow));
            }
        } else {
            UDSResponseParseCheckBox->setEnabled(true);
            cycleNumberEdit->setEnabled(true);
            loadTestProjectBtn->setEnabled(true);
            serviceSetclearBtn->setEnabled(true);
            addServiceBtn->setEnabled(true);
            resultFileName.clear();
        }
    });

    QPushButton *UDSProjectListShowBtn = new QPushButton("服务集合测试");
    UDSProjectListShowBtn->setIcon(QIcon(":/icon/expand.png"));
    connect(UDSProjectListShowBtn, &QPushButton::clicked, this, [this, UDSProjectListShowBtn]{
        if (ProjectListWidget->isHidden()) {
            ProjectListWidget->show();
            UDSProjectListShowBtn->setIcon(QIcon(":/icon/zoom.png"));
        } else {
            ProjectListWidget->hide();
            UDSProjectListShowBtn->setIcon(QIcon(":/icon/expand.png"));
        }
    });
    udsOpInfoLayout->addWidget(UDSProjectListShowBtn);

    sendListBtn = new QPushButton("开始测试");
    sendListBtn->setIcon(QIcon(":/icon/testing.png"));
    udsOpInfoLayout->addWidget(sendListBtn);
    connect(sendListBtn, &QPushButton::clicked, this, [this]{
        if (serviceSetModel->rowCount() == 0) {
            return ;
        }

        if ((doipDiagCheckBox->checkState() != Qt::Checked || \
            doipServerListBox->currentText().size() == 0) &&\
            adbDiagCheckBox->checkState() != Qt::Checked &&\
            (canDiagCheckBox->checkState() != Qt::Checked || \
             can->getCurrChannel().size() == 0)) {
            promptForAction(QString("<font color = red>未选择有效UDS通道</font>"));
            return ;
        }
        // CANDiagPcap.startCANCapturePackets("");
        addDidDescBtn->setEnabled(false);
        addDtcDescBtn->setEnabled(false);
        addServiceBtn->setEnabled(false);
        transfer36MaxlenEdit->setEnabled(false);
        serviceSetResetBtn->setEnabled(false);
        loadTestProjectBtn->setEnabled(false);
        saveTestProjectBtn->setEnabled(false);
        delTestProjectBtn->setEnabled(false);
        serviceSetclearBtn->setEnabled(false);
        testResultCheckBox->setEnabled(false);
        cycleNumberEdit->setEnabled(false);
        TestProjectNameEdit->setEnabled(false);
        adbDiagCheckBox->setEnabled(false);
        canDiagCheckBox->setEnabled(false);
        doipDiagCheckBox->setEnabled(false);
        allServiceTimer.restart();
        serviceTimer.restart();
        progressWidget->setHidden(false);
        sendListBtn->setEnabled(false);
        sendAbortListBtn->setEnabled(true);
        udsViewSetColumnHidden(true);
        serviceActiveTask.runtotal = 0;
        serviceActiveTask.indexRow = 0;
        if (adbDiagCheckBox->checkState() == Qt::Checked) {
            serviceActiveTask.channelName = adbDiagChannelName;
        } else if (canDiagCheckBox->checkState() == Qt::Checked){
            serviceActiveTask.channelName = can->getCurrChannel();
        } else if (doipDiagCheckBox->checkState() == Qt::Checked) {
            serviceActiveTask.channelName = doipServerListBox->currentText();
            if (IPDiagPcapCheckBox->checkState() == Qt::Checked) {
                diagPcap.startIPCapturePackets(QHostAddress(serviceActiveTask.channelName.split("/").at(0)));
            }
        }
        getServiceItemData(serviceActiveTask.indexRow, serviceActiveTask.data);
        serviceActiveTask.cyclenum = 0;
        serviceActiveTask.cyclenum = cycleNumberEdit->text().toUInt();
        if (serviceActiveTask.cyclenum > 0) {
            serviceActiveTask.cyclenum--; /* 这里表示剩余的测试次数，默认剩余测试次数为0，就是只测试一次 */
        }
        serviceListTaskTimer->setInterval(serviceActiveTask.data.delay);
        serviceListTaskTimer->start();

        /* 传输速度计算 */
        serviceActiveTask.upPreTime = QDateTime::currentDateTime();
        serviceActiveTask.upAverageByte = 0;
        serviceActiveTask.upTotalByte = 0;
        upTransferSpeedLable->setText("0 KB/s");

        serviceActiveTask.downPreTime = QDateTime::currentDateTime();
        serviceActiveTask.downAverageByte = 0;
        serviceActiveTask.downTotalByte = 0;
        downTransferSpeedLable->setText("0 KB/s");

        serviceListReset(serviceSetModel);
#if 1
        QList<QStandardItem *> itemList;
        for (int nn = 0; nn < serviceSetModel->columnCount(); nn++) {
            QStandardItem *item = serviceSetModel->item(serviceActiveTask.indexRow, nn);
            itemList.append(item);
        }
        setActiveServiceFlicker(itemList);
#endif
        promptForAction(QString("<font color = green>测试准备开始</font>"));
    });
    sendAbortListBtn = new QPushButton("终止测试");
    sendAbortListBtn->setEnabled(false);
    sendAbortListBtn->setIcon(QIcon(":/icon/stop.png"));
    udsOpInfoLayout->addWidget(sendAbortListBtn);
    connect(sendAbortListBtn, &QPushButton::clicked, this, [this]{
        if (serviceSetActive()) {
            promptForAction(QString("<font color = red>集合测试中，无法中止</font>"));
        } else {
            if (testResultCheckBox->checkState() == Qt::Checked) {
                QString prefix("手动终止测试 " + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss"));
                tableToXlsx.modelToXlsx(*serviceSetModel, resultFileName, prefix, udsViewResultColumn);
                testResultCheckBox->setCheckState(Qt::Unchecked);
            }
            abortServiceTask();
            promptForAction(QString("<font color = #FF6103>测试终止</font>"));
        }
    });

    QHBoxLayout *infoShowLayout = new QHBoxLayout();
    mainLayout->addLayout(infoShowLayout);

    failureAbortCheckBox = new QCheckBox("失败终止");
    failureAbortCheckBox->setToolTip("有失败项将终止测试");
    infoShowLayout->addWidget(failureAbortCheckBox);

    IPDiagPcapCheckBox = new QCheckBox("pcap保存");
    IPDiagPcapCheckBox->setToolTip("将收集保存所有的doip诊断报文");
    infoShowLayout->addWidget(IPDiagPcapCheckBox);
    connect(IPDiagPcapCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (Qt::Checked != state) {
            // highPerformanceEnable = false;
            // promptForAction(QString("<font color = red>高性能模式关闭，开启更多的UI动画将影响测试性能</font>"));
        } else {
            // highPerformanceEnable = true;
            // promptForAction(QString("<font color = green>高性能模式开启，将减少UI动画提高测试性能</font>"));
        }
    });

    highPerformanceCheckBox = new QCheckBox("高性能模式");
    highPerformanceCheckBox->setToolTip("将减少UI动画提高测试性能");
    infoShowLayout->addWidget(highPerformanceCheckBox);
    connect(highPerformanceCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (Qt::Checked != state) {
            highPerformanceEnable = false;
            promptForAction(QString("<font color = red>高性能模式关闭，开启更多的UI动画将影响测试性能</font>"));
        } else {
            highPerformanceEnable = true;
            promptForAction(QString("<font color = green>高性能模式开启，将减少UI动画提高测试性能</font>"));
        }
    });
    UDSResponseParseCheckBox = new QCheckBox("UDS协议解析");
    UDSResponseParseCheckBox->setToolTip("自动解析UDS应答数据");
    infoShowLayout->addWidget(UDSResponseParseCheckBox);
    connect(UDSResponseParseCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        int column[] = {udsViewResponseColumn, udsViewExpectResponseColumn};
        int columnHeight = 30;
        serviceSetView->verticalHeader()->setMinimumSectionSize(30);
        for (int index = 0; index < serviceSetModel->rowCount(); index++) {
            for (unsigned int cc = 0; cc < sizeof(column) / sizeof(column[0]); cc++) {
                QStandardItem *item = serviceSetModel->item(index, column[cc]);
                if (item && item->text().count('\n')) {
                    if (Qt::Checked != state) {
                        QString parsedata = item->toolTip();
                        item->setToolTip(item->text());
                        item->setText(parsedata);
                    }
                } else {
                    if (Qt::Checked == state) {
                        QByteArray bytedata = QByteArray::fromHex(item->text().remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                        QString parseStr = udsParse(bytedata);
                        item->setText(parseStr);
                        item->setToolTip(QString(bytedata.toHex(' ')));
                        columnHeight = fontHeight *  parseStr.count('\n') > columnHeight ? fontHeight *  parseStr.count('\n'): columnHeight;
                    }
                }
            }
            serviceSetView->verticalHeader()->resizeSection(index, columnHeight);
        }
        udsViewSetColumnHidden(true);
    });
    serviceSetclearBtn = new QPushButton("清除列表");
    serviceSetclearBtn->setIcon(QIcon(":/icon/clear.png"));
    infoShowLayout->addWidget(serviceSetclearBtn);
    connect(serviceSetclearBtn, &QPushButton::clicked, this, [this](){
        switch (QMessageBox::information(0, tr("删除"), tr("确认清除列表?"), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default: return ; break;
        }
        serviceSetModel->removeRows(0, serviceSetModel->rowCount());
    });

    serviceSetResetBtn = new QPushButton("重置列表");
    serviceSetResetBtn->setIcon(QIcon(":/icon/reset.png"));
    infoShowLayout->addWidget(serviceSetResetBtn);
    connect(serviceSetResetBtn, &QPushButton::clicked, this, [this](){
        switch (QMessageBox::information(0, tr("重置"), tr("确认重置列表?"), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default: return ; break;
        }
        serviceListReset(serviceSetModel);
    });
    infoShowLayout->addStretch();
    udsOpInfoLayout->addStretch();
    udsProjectLayout->addStretch();
    mainLayout->addStretch();

    return mainWidget;
}

QWidget *UDS::uiConfigWidget()
{
    QWidget *mainWidget = new QWidget();
    QVBoxLayout *mainWidgetLayout = new QVBoxLayout();
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainWidgetLayout->addLayout(mainLayout);
    mainWidget->setLayout(mainWidgetLayout);

    UDSDescolumnCheckBox = new QCheckBox("备注");
    mainLayout->addWidget(UDSDescolumnCheckBox);
    UDSDescolumnCheckBox->setCheckState(Qt::Checked);
    connect(UDSDescolumnCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (serviceSetView == nullptr) return ;
        if (Qt::Checked == state) {
            serviceSetView->setColumnHidden(udsViewRemarkColumn, false);
        } else {
            serviceSetView->setColumnHidden(udsViewRemarkColumn, true);
        }
    });

    SendBtnCheckBox = new QCheckBox("发送");
    mainLayout->addWidget(SendBtnCheckBox);
    SendBtnCheckBox->setCheckState(Qt::Checked);
    connect(SendBtnCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (serviceSetView == nullptr) return ;
        if (Qt::Checked == state) {
            serviceSetView->setColumnHidden(udsViewSendBtnColumn, false);
        } else {
            serviceSetView->setColumnHidden(udsViewSendBtnColumn, true);
        }
    });

    ConfigBtnCheckBox = new QCheckBox("配置");
    mainLayout->addWidget(ConfigBtnCheckBox);
    ConfigBtnCheckBox->setCheckState(Qt::Checked);
    connect(ConfigBtnCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (serviceSetView == nullptr) return ;
        if (Qt::Checked == state) {
            serviceSetView->setColumnHidden(udsViewConfigBtnColumn, false);
        } else {
            serviceSetView->setColumnHidden(udsViewConfigBtnColumn, true);
        }
    });

    UpBtnCheckBox = new QCheckBox("上移");
    mainLayout->addWidget(UpBtnCheckBox);
    UpBtnCheckBox->setCheckState(Qt::Checked);
    connect(UpBtnCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (serviceSetView == nullptr) return ;
        if (Qt::Checked == state) {
            serviceSetView->setColumnHidden(udsViewUpBtnColumn, false);
        } else {
            serviceSetView->setColumnHidden(udsViewUpBtnColumn, true);
        }
    });

    DownBtnCheckBox = new QCheckBox("下移");
    mainLayout->addWidget(DownBtnCheckBox);
    DownBtnCheckBox->setCheckState(Qt::Checked);
    connect(DownBtnCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (serviceSetView == nullptr) return ;
        if (Qt::Checked == state) {
            serviceSetView->setColumnHidden(udsViewDownBtnColumn, false);
        } else {
            serviceSetView->setColumnHidden(udsViewDownBtnColumn, true);
        }
    });

    DelBtnCheckBox = new QCheckBox("删除");
    mainLayout->addWidget(DelBtnCheckBox);
    DelBtnCheckBox->setCheckState(Qt::Checked);
    connect(DelBtnCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (serviceSetView == nullptr) return ;
        if (Qt::Checked == state) {
            serviceSetView->setColumnHidden(udsViewDelBtnColumn, false);
        } else {
            serviceSetView->setColumnHidden(udsViewDelBtnColumn, true);
        }
    });

    serviceTestResultCheckBox = new QCheckBox("图表统计");
    mainLayout->addWidget(serviceTestResultCheckBox);
    serviceTestResultCheckBox->setCheckState(Qt::Unchecked);
#ifdef __HAVE_CHARTS__
    connect(serviceTestResultCheckBox, &QCheckBox::stateChanged, this, [this](int state){
        if (chartView == nullptr) return ;
        if (Qt::Checked != state) {
            chartView->hide();
        } else {
            chartView->show();
        }
    });
#endif /* __HAVE_CHARTS__ */
    mainLayout->addStretch();
    mainWidgetLayout->addStretch();

    return mainWidget;
}

QWidget *UDS::serviceGeneralConfigWidget()
{
    QWidget *mainWidget = new QWidget();
    QVBoxLayout *mainWidgetLayout = new QVBoxLayout();
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainWidgetLayout->addLayout(mainLayout);
    mainWidget->setLayout(mainWidgetLayout);
    mainLayout->addWidget(new QLabel("36传输字节:"));
    transfer36MaxlenEdit = new QLineEdit();
    transfer36MaxlenEdit->setValidator(new QIntValidator(0, 100000, transfer36MaxlenEdit));
    transfer36MaxlenEdit->setMaximumWidth(60);
    mainLayout->addWidget(transfer36MaxlenEdit);
    connect(transfer36MaxlenEdit, &QLineEdit::textChanged, this, [this](const QString &text){
        transfer36Maxlen = text.toUInt();
    });

    QCheckBox *enable3e = new QCheckBox("TesterPresent(3E)");
    mainLayout->addWidget(enable3e);
    connect(enable3e, &QCheckBox::stateChanged, this, [this](int state){
        if (cycle3eTimer == nullptr || cycle3eEdit == nullptr) return ;
        if (Qt::Checked == state) {
            if (cycle3eEdit->text().size() == 0) {
                cycle3eEdit->setText("2000");
            }
            cycle3eTimer->setInterval(cycle3eEdit->text().toUInt());
            cycle3eTimer->start();
            cycle3eEdit->setEnabled(false);
        } else {
            cycle3eTimer->stop();
            cycle3eEdit->setEnabled(true);
        }
    });
    mainLayout->addWidget(new QLabel("发送周期(ms):"));
    cycle3eEdit = new QLineEdit();
    cycle3eEdit->setValidator(new QIntValidator(0, 100000, cycleNumberEdit));
    cycle3eEdit->setMaximumWidth(40);
    cycle3eEdit->setText("2000");
    mainLayout->addWidget(cycle3eEdit);

    mainLayout->addWidget(new QLabel("源地址:"));
    saAdress3eEdit = new QLineEdit();
    saAdress3eEdit->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{8}$"), this));
    saAdress3eEdit->setMaximumWidth(70);
    mainLayout->addWidget(saAdress3eEdit);

    mainLayout->addWidget(new QLabel("目的地址:"));
    taAdress3eEdit = new QLineEdit();
    taAdress3eEdit->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{8}$"), this));
    taAdress3eEdit->setMaximumWidth(70);
    mainLayout->addWidget(taAdress3eEdit);

    suppress3eCheckBox = new QCheckBox("抑制响应");
    mainLayout->addWidget(suppress3eCheckBox);
    suppress3eCheckBox->setCheckState(Qt::Checked);

    fixCycle3eCheckBox = new QCheckBox("固定周期");
    mainLayout->addWidget(fixCycle3eCheckBox);
    fixCycle3eCheckBox->setCheckState(Qt::Checked);

    cycle3eTimer = new QTimer();
    connect(cycle3eTimer, &QTimer::timeout, this, [this]{
        bool isspuress = suppress3eCheckBox->checkState() == Qt::Checked ? true :false;
        quint32 ta = taAdress3eEdit->text().toUInt(nullptr, 16);
        quint32 sa = taAdress3eEdit->text().toUInt(nullptr, 16);
        char msg[] = {0x3e, 0x00};
        if (isspuress) {
            msg[1] = 0x80;
        }
        if (ta == 0) return ;

        if (doipDiagCheckBox->checkState() == Qt::Checked) {
            if (doipServerListBox->currentText().size() > 0) {
                DoipClientConnect *conn = doipConnMap.find(doipServerListBox->currentText()).value();
                if (conn) {
                    conn->diagnosisRequest(msg, sizeof(msg), ta, isspuress, 0);
                }
            }
        } else if (canDiagCheckBox->checkState() == Qt::Checked){
            if (can) {
                can->sendRequest(msg, sizeof(msg), sa, ta, isspuress, 0);
            }
        } else {
            adbDiagRequest(msg, sizeof(msg), ta, isspuress, 0);
        }
    });
    mainLayout->addStretch();
    mainWidgetLayout->addStretch();

    return mainWidget;
}

QWidget *UDS::serviceConfigWidget()
{
    QWidget *configWidget = new QWidget();
    configWidget->setAttribute(Qt::WA_QuitOnClose, false);
    QVBoxLayout *configMainLayout = new QVBoxLayout();

    configWidget->setWindowIcon(QIcon(":/icon/reset.png"));
    configWidget->setWindowTitle("诊断项配置");

    configWidget->setStyleSheet(".QWidget{background-color: #FFFFFF;}" + btnCommonStyle + comboxCommonStyle);

    udsRequestConfigBtn = new QPushButton("诊断服务添加至列表");
    udsRequestConfigBtn->setIcon(QIcon(":/icon/addList.png"));
    configMainLayout->addWidget(udsRequestConfigBtn);
    udsRequestConfigBtn->setStyleSheet(pushButtonStyle);

    QGroupBox *UDSRequestBox = new QGroupBox("服务请求规则");
    QVBoxLayout *UDSRequestLayout = new QVBoxLayout();
    QVBoxLayout *requestServiceConfigLayout = new QVBoxLayout();
    UDSRequestBox->setLayout(UDSRequestLayout);
    UDSRequestLayout->addLayout(requestServiceConfigLayout);
    configMainLayout->addWidget(UDSRequestBox);
    configWidget->setLayout(configMainLayout);

    QGridLayout *serviceParamLayout = new QGridLayout();
    requestServiceConfigLayout->addLayout(serviceParamLayout);

    serviceParamLayout->addWidget(new QLabel("服务ID:"), 0, 0, 1, 1);
    serviceListBox = new QComboBox();
    serviceListBox->setFixedWidth(300);
    serviceEidt = new QLineEdit();
    serviceEidt->setFixedWidth(30);
    serviceEidt->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{2}$"), this));
    for (quint8 sid = 0; sid < 0xff; sid++) {
        if (serviceDescMap.contains(sid)) {
            serviceListBox->addItem(serviceDescMap[sid]);
        }
    }
    serviceParamLayout->addWidget(serviceListBox, 0, 1, 1, 3);
    serviceParamLayout->addWidget(serviceEidt, 0, 4, 1, 1);

    serviceParamLayout->addWidget(new QLabel("子功能:"), 1, 0, 1, 1);
    serviceSubListBox = new QComboBox();
    serviceSubListBox->setFixedWidth(300);
    serviceParamLayout->addWidget(serviceSubListBox, 1, 1, 1, 3);
    serviceSubEidt = new QLineEdit();
    serviceSubEidt->setFixedWidth(30);
    serviceParamLayout->addWidget(serviceSubEidt, 1, 4, 1, 1);
    serviceSubEidt->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{2}$"), this));

    //QGridLayout *otherParamLayout = new QGridLayout();
    //requestServiceConfigLayout->addLayout(otherParamLayout);

    serviceParamLayout->addWidget(new QLabel("数据标识符:"), 2, 0);
    serviceDidEidt = new QLineEdit();
    serviceDidEidt->setFixedWidth(60);
    serviceParamLayout->addWidget(serviceDidEidt, 2, 1);
    serviceDidEidt->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{4}$"), this));

    serviceParamLayout->addWidget(new QLabel("目的地址:"), 2, 2);
    serviceTaEdit = new QLineEdit();
    serviceTaEdit->setFixedWidth(60);
    serviceParamLayout->addWidget(serviceTaEdit, 2, 3);
    serviceTaEdit->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{4}$"), this));

    serviceAddrType = new QComboBox();
    serviceAddrType->addItems(QStringList() << "physical address" << "function address");
    serviceParamLayout->addWidget(serviceAddrType, 2, 4);

    serviceParamLayout->addWidget(new QLabel("超时时间(ms):"), 3, 0);
    serviceTimeoutEdit = new QLineEdit();
    serviceTimeoutEdit->setFixedWidth(60);
    serviceParamLayout->addWidget(serviceTimeoutEdit, 3, 1);
    serviceTimeoutEdit->setValidator(new QIntValidator(0, 10000, serviceTimeoutEdit));

    serviceParamLayout->addWidget(new QLabel("延时时间(ms):"), 3, 2);
    serviceDelayEdit = new QLineEdit();
    serviceDelayEdit->setFixedWidth(60);
    serviceParamLayout->addWidget(serviceDelayEdit, 3, 3);
    serviceDelayEdit->setValidator(new QIntValidator(0, 100000, serviceDelayEdit));

    serviceSuppressCheck = new QCheckBox("抑制响应");
    serviceParamLayout->addWidget(serviceSuppressCheck, 3, 4);
    connect(serviceSuppressCheck, &QCheckBox::stateChanged, this, [this](int state) {
        if (state == Qt::Checked) {
            serviceTimeoutEdit->setText("200");
        }
    });

    fileTransferWidget = requestFileTransferWidget();
    requestServiceConfigLayout->addWidget(fileTransferWidget);
    securityWidget = securityAccessWidget();
    requestServiceConfigLayout->addWidget(securityWidget);

    diagReqMsgEdit = new QPlainTextEdit();
    requestServiceConfigLayout->addWidget(diagReqMsgEdit);

    QHBoxLayout *msgSetLayout = new QHBoxLayout();
    UDSRequestLayout->addLayout(msgSetLayout);
    msgSetLayout->addWidget(new QLabel("字符(HEX):"));
    QLineEdit *diagRandCharEdit = new QLineEdit();
    diagRandCharEdit->setValidator(new QRegExpValidator(QRegExp("^[a-fA-F0-9]{2}$"), this));
    diagRandCharEdit->setMaximumWidth(30);
    msgSetLayout->addWidget(diagRandCharEdit);
    msgSetLayout->addStretch();

    msgSetLayout->addWidget(new QLabel("长度(byte):"));
    QLineEdit *diagRandMsgLenEdit = new QLineEdit();
    diagRandMsgLenEdit->setValidator(new QRegExpValidator(QRegExp("^[0-9]{5}$"), this));
    diagRandMsgLenEdit->setMaximumWidth(50);
    msgSetLayout->addWidget(diagRandMsgLenEdit);

    QPushButton *diagRandMsgBtn = new QPushButton("生成");
    diagRandMsgBtn->setIcon(QIcon(":/icon/generate.png"));
    msgSetLayout->addWidget(diagRandMsgBtn);

    QPushButton *diagCleanBtn = new QPushButton("清除");
    diagCleanBtn->setIcon(QIcon(":/icon/empty.png"));
    msgSetLayout->addWidget(diagCleanBtn);

    QLabel *diagMsgLen = new QLabel("0/byte");
    diagMsgLen->setFixedWidth(60);
    msgSetLayout->addWidget(diagMsgLen);

    connect(diagCleanBtn, &QPushButton::clicked, this, [this, diagMsgLen] {
        diagReqMsgEdit->clear();
        diagMsgLen->setText("0/byte");
    });

    connect(diagRandMsgBtn, &QPushButton::clicked, this, [this, diagRandMsgLenEdit, diagRandCharEdit] {
        QString randMsg;

        if (diagRandCharEdit->text().size() == 1) {
            diagRandCharEdit->setText("0" + diagRandCharEdit->text());
        }
        randMsg.append(diagReqMsgEdit->toPlainText());
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
    diagReqMsgEdit->setMaximumHeight(40);
    connect(diagReqMsgEdit, &QPlainTextEdit::textChanged, this, [this, diagMsgLen]{
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

    serviceDesc = new QLineEdit();
    UDSRequestLayout->addWidget(serviceDesc);
    serviceDesc->setPlaceholderText("诊断服务描述");

    connect(serviceSubListBox, &QComboBox::currentTextChanged, this, [this](const QString &text){
        if (serviceDescMap.contains(serviceEidt->text().toInt(nullptr, 16)) &&\
            serviceSubMap.contains(serviceEidt->text().toInt(nullptr, 16))) {
            serviceSubEidt->setText(QString(QByteArray(1, serviceSubMap[serviceEidt->text().toInt(nullptr, 16)][text]).toHex()));
        }
    });
    connect(serviceSubEidt, &QLineEdit::textChanged, this, [this](const QString &text){
        diagReqMsgEdit->setPlainText(serviceRequestEncode().toHex(' '));
        if (serviceDescMap.contains(serviceEidt->text().toInt(nullptr, 16)) &&\
            serviceSubDescMap.contains(serviceEidt->text().toInt(nullptr, 16))) {
            serviceSubListBox->setCurrentText(serviceSubDescMap[serviceEidt->text().toInt(nullptr, 16)][text.toInt(nullptr, 16)]);
        }
    });
    connect(serviceListBox, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        serviceDidEidt->clear();
        serviceDesc->clear();
        if (serviceMap.contains(text)) {
            serviceEidt->setText(QString(QByteArray(1, serviceMap[text]).toHex()));
            serviceSubListBox->clear();
            serviceSubEidt->clear();
            if (serviceMap.contains(text) && serviceSubDescMap.contains(serviceMap[text])) {
                serviceSubListBox->setEnabled(true);
                serviceSubEidt->setEnabled(true);
                for (quint8 sub = 0; sub < 0xff; sub++) {
                    if (serviceSubDescMap[serviceMap[text]].contains(sub)) {
                        if (serviceSubDescMap[serviceMap[text]][sub].size()) {
                            serviceSubListBox->addItem(serviceSubDescMap[serviceMap[text]][sub]);
                        }
                    }
                }
            }
            else {
                serviceSubListBox->setEnabled(false);
                serviceSubEidt->setEnabled(false);
            }
        }
        if (serviceExpectRespBox) {
            serviceExpectRespBox->setCurrentText(responseExpectDesc[notSetResponseExpect]);
        }
        if (serviceRespPlainText) {
            serviceRespPlainText->clear();
            serviceRespPlainText->setPlainText(QString::number(serviceEidt->text().toInt(0, 16) + 0x40, 16));
        }
        if (serviceConfigWidgetRefreshTimer) {
            serviceConfigWidgetRefreshTimer->setInterval(2);
            serviceConfigWidgetRefreshTimer->start();
        }
    });
    connect(serviceEidt, &QLineEdit::textChanged, this, [this](const QString &text){
        diagReqMsgEdit->setPlainText(serviceRequestEncode().toHex(' '));
        if (serviceDescMap.contains(text.toInt(nullptr, 16))) {
            serviceListBox->setCurrentText(serviceDescMap[text.toInt(nullptr, 16)]);
        }
        if (text.toInt(nullptr, 16) == RequestFileTransfer) {
            fileTransferWidget->show();
        } else {
            fileTransferWidget->hide();
        }
        if (text.toInt(nullptr, 16) == SecurityAccess) {
            securityWidget->show();
        } else {
            securityWidget->hide();
        }
        if (serviceConfigWidgetRefreshTimer) {
            serviceConfigWidgetRefreshTimer->setInterval(2);
            serviceConfigWidgetRefreshTimer->start();
        }
    });

    connect(serviceSuppressCheck, &QCheckBox::stateChanged, this, [this](int state){
        (void)state;
        diagReqMsgEdit->setPlainText(serviceRequestEncode().toHex(' '));
    });
    connect(serviceDidEidt, &QLineEdit::textChanged, this, [this](const QString &text){
        (void)text;
        diagReqMsgEdit->setPlainText(serviceRequestEncode().toHex(' '));
    });

    serviceListBox->setCurrentIndex(1);
    serviceListBox->setCurrentIndex(0);

    QGroupBox *UDSResponseBox = new QGroupBox("服务响应校验规则");
    QVBoxLayout *UDSResponseLayout = new QVBoxLayout();
    QGridLayout *responseServiceConfigLayout = new QGridLayout();
    UDSResponseBox->setLayout(UDSResponseLayout);
    UDSResponseLayout->addLayout(responseServiceConfigLayout);
    configMainLayout->addWidget(UDSResponseBox);

    responseServiceConfigLayout->addWidget(new QLabel("预期响应："), 0, 0);
    serviceExpectRespBox = new QComboBox();
    QMap<QString, UDS::serviceResponseExpect>::Iterator it;
    for (it = responseExpectMap.begin(); it != responseExpectMap.end(); ++it) {
        serviceExpectRespBox->addItem(it.key());
    }
    responseServiceConfigLayout->addWidget(serviceExpectRespBox, 0, 1);
    serviceExpectRespBox->setCurrentText(responseExpectDesc[notSetResponseExpect]);

    responseServiceConfigLayout->addWidget(new QLabel("消极响应码："), 1, 0);
    negativeResponseCodeBox = new QComboBox();
    for (quint8 code = 0; code < 0xff; code++) {
        if (positiveResponseCodeDesc.contains(code)) {
            negativeResponseCodeBox->addItem(positiveResponseCodeDesc[code]);
        }
    }
    negativeResponseCodeBox->setEnabled(false);
    responseServiceConfigLayout->addWidget(negativeResponseCodeBox, 1, 1);

    serviceRespPlainText = new QPlainTextEdit();
    serviceRespPlainText->setEnabled(false);
    serviceRespPlainText->setMaximumHeight(40);
    UDSResponseLayout->addWidget(serviceRespPlainText);
    connect(serviceRespPlainText, &QPlainTextEdit::textChanged, this, [this]{
        if (responseExpectMap[serviceExpectRespBox->currentText()] == responseRegexMatch || \
            responseExpectMap[serviceExpectRespBox->currentText()] == responseRegexMismatch) {
            return ;
        }

        QString editText = serviceRespPlainText->toPlainText();
        QString regStr(editText.remove(QRegExp("[^0-9a-fA-F^ ]")));
        if (regStr.length() != serviceRespPlainText->toPlainText().length()) {
            if (regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8().length() % 2 == 0) {
                QByteArray msg = QByteArray::fromHex(regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                serviceRespPlainText->setPlainText(QString(msg.toHex(' ')));
            }
            else {
                serviceRespPlainText->setPlainText(editText);
            }
            serviceRespPlainText->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
        }
        else {
            if (regStr.contains(QRegExp("[0-9a-f]{4}"))) {
                if (regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8().length() % 2 == 0) {
                    QByteArray msg = QByteArray::fromHex(regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                    serviceRespPlainText->setPlainText(QString(msg.toHex(' ')));
                    serviceRespPlainText->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
                }
            }
        }
    });
    connect(serviceExpectRespBox, &QComboBox::currentTextChanged, this, [this](const QString &text){
        serviceRespPlainText->setEnabled(true);
        serviceRespPlainText->clear();
        if (responseExpectMap[text] != negativeResponseExpect) {
            negativeResponseCodeBox->setEnabled(false);
            if (responseExpectMap[text] == noResponseExpect ||\
                responseExpectMap[text] == notSetResponseExpect) {
                serviceRespPlainText->setEnabled(false);
            }
        } else {
            QByteArray expectResponse;

            expectResponse.append(0x7f);
            expectResponse.append(serviceEidt->text().toUInt(0, 16));
            expectResponse.append(positiveResponseCodeMap[negativeResponseCodeBox->currentText()]);
            serviceRespPlainText->setPlainText(expectResponse.toHex(' '));
            negativeResponseCodeBox->setEnabled(true);
            serviceRespPlainText->setEnabled(false);
        }
    });

    connect(negativeResponseCodeBox, &QComboBox::currentTextChanged, this, [this](const QString &text){
            QByteArray expectResponse;

            expectResponse.append(0x7f);
            expectResponse.append(serviceEidt->text().toUInt(0, 16));
            expectResponse.append(positiveResponseCodeMap[text]);
            serviceRespPlainText->setPlainText(expectResponse.toHex(' '));
            negativeResponseCodeBox->setEnabled(true);
    });

    QGroupBox *UDSFinishBox = new QGroupBox("服务结束规则");
    QVBoxLayout *UDSFinishLayout = new QVBoxLayout();
    UDSFinishBox->setLayout(UDSFinishLayout);
    configMainLayout->addWidget(UDSFinishBox);

    QHBoxLayout *UDSRuleLayout = new QHBoxLayout();
    UDSFinishLayout->addLayout(UDSRuleLayout);

    UDSRuleLayout->addWidget(new QLabel("结束条件:"));
    UDSConditionRuleComboBox = new QComboBox();
    UDSConditionRuleComboBox->addItems(QStringList() << "default setting" << "equal to" << "unequal to" << "regex match" << "regex mismatch");
    UDSConditionMap.insert("default setting", UDSFinishDefaultSetting);
    UDSConditionMap.insert("equal to", UDSFinishEqualTo);
    UDSConditionMap.insert("unequal to", UDSFinishUnEqualTo);
    UDSConditionMap.insert("regex match", UDSFinishRegexMatch);
    UDSConditionMap.insert("regex mismatch", UDSFinishRegexMismatch);
    UDSConditionMap.insert("regular expression matching", UDSFinishRegularExpressionMatch);
    UDSRuleLayout->addWidget(UDSConditionRuleComboBox);
    UDSRuleLayout->addStretch();

    UDSRuleLayout->addWidget(new QLabel("允许最大重复次数:"));
    UDSFinishRuleEdit = new QLineEdit();
    UDSFinishRuleEdit->setValidator(new QRegExpValidator(QRegExp("^[0-9]{10}$"), this));
    UDSFinishRuleEdit->setMaximumWidth(100);
    UDSRuleLayout->addWidget(UDSFinishRuleEdit);
    UDSRuleLayout->addWidget(new QLabel(QString("<font color = red>※防止无限循环</font>")));

    UDSFinishPlainText = new QPlainTextEdit();
    UDSFinishLayout->addWidget(UDSFinishPlainText);
    UDSFinishPlainText->setEnabled(false);
    UDSFinishPlainText->setMaximumHeight(40);
    UDSFinishLayout->addWidget(UDSFinishPlainText);
    connect(UDSFinishPlainText, &QPlainTextEdit::textChanged, this, [this]{
        if (UDSConditionMap[UDSConditionRuleComboBox->currentText()] == UDSFinishRegularExpressionMatch || \
            UDSConditionMap[UDSConditionRuleComboBox->currentText()] == UDSFinishRegexMatch ||\
            UDSConditionMap[UDSConditionRuleComboBox->currentText()] == UDSFinishRegexMismatch) {
            return ;
        }

        QString editText = UDSFinishPlainText->toPlainText();
        QString regStr(editText.remove(QRegExp("[^0-9a-fA-F^ ]")));
        if (regStr.length() != UDSFinishPlainText->toPlainText().length()) {
            if (regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8().length() % 2 == 0) {
                QByteArray msg = QByteArray::fromHex(regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                UDSFinishPlainText->setPlainText(QString(msg.toHex(' ')));
            }
            else {
                UDSFinishPlainText->setPlainText(editText);
            }
            UDSFinishPlainText->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
        }
        else {
            if (regStr.contains(QRegExp("[0-9a-f]{4}"))) {
                if (regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8().length() % 2 == 0) {
                    QByteArray msg = QByteArray::fromHex(regStr.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                    UDSFinishPlainText->setPlainText(QString(msg.toHex(' ')));
                    UDSFinishPlainText->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
                }
            }
        }
    });

    connect(UDSConditionRuleComboBox, &QComboBox::currentTextChanged, this, [this](const QString &text){
        if (UDSConditionMap[text] != UDSFinishDefaultSetting) {
            UDSFinishPlainText->setEnabled(true);
        } else {
            UDSFinishPlainText->setEnabled(false);
        }
    });

    connect(udsRequestConfigBtn, &QPushButton::clicked, this, [this]{
        serviceItem data;

        if (!diagReqMsgEdit->toPlainText().size()) {
            return ;
        }
        QString dataString = widgetConfigToString();
        data = serviceItemDataDecode(dataString);
        if (data.timeout == 0) {serviceTimeoutEdit->setText("2000");}
        serviceDelayEdit->setText(QString("%1").arg(data.delay));
        if (data.ta == 0) {
            QString doipName = doipServerListBox->currentText();
            if (doipConnMap.contains(doipName)) {
                DoipClientConnect *conn = doipConnMap.find(doipName).value();
                if (conn) {
                    serviceTaEdit->setText(QString::number(conn->getLogicTargetAddress(), 16));
                }
            }
        }
        dataString = widgetConfigToString();
        data = serviceItemDataDecode(dataString);
        if (!(modifyRowIndex < 0)) {
            modifyServiceItem(modifyRowIndex, data);
            modifyRowIndex = -1;
            serviceSetWidget->close();
        }
        else {
            addServiceItem(data);
        }
    });
    serviceConfigWidgetRefreshTimer = new QTimer();
    connect(serviceConfigWidgetRefreshTimer, &QTimer::timeout, this, [this]{
        serviceConfigWidgetRefreshTimer->stop();
        if (serviceSetWidget) {
            serviceSetWidget->resize(1, 1);
        }
    });

    return configWidget;
}

QWidget *UDS::requestFileTransferWidget()
{
    QWidget *mainWidget = new QWidget();
    QGridLayout *mainLayout = new QGridLayout();
    mainWidget->setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    FileSelect *fileSelectWidget = new FileSelect();

    mainLayout->addWidget(new QLabel("操作方式:"), 0, 0);
    modeOfOperationComBox = new QComboBox();
    modeOfOperationComBox->addItems(QStringList() \
                                    << "AddFile(0x01)" \
                                    << "DeleteFile(0x02)"\
                                    << "ReplaceFile(0x03)" \
                                    << "ReadFile(0x04)" \
                                    << "ReadDir(0x05)");
    mainLayout->addWidget(modeOfOperationComBox, 0, 1);
    modeOfOperationMap["AddFile(0x01)"] = AddFile;
    modeOfOperationMap["DeleteFile(0x02)"] = DeleteFile;
    modeOfOperationMap["ReplaceFile(0x03)"] = ReplaceFile;
    modeOfOperationMap["ReadFile(0x04)"] = ReadFile;
    modeOfOperationMap["ReadDir(0x05)"] = ReadDir;

    modeOfOperationDescMap[AddFile] = "AddFile(0x01)";
    modeOfOperationDescMap[DeleteFile] = "DeleteFile(0x02)";
    modeOfOperationDescMap[ReplaceFile] = "ReplaceFile(0x03)";
    modeOfOperationDescMap[ReadFile] = "ReadFile(0x04)";
    modeOfOperationDescMap[ReadDir] = "ReadDir(0x05)";

    mainLayout->addWidget(new QLabel("文件路径和文件名长度:"), 0, 2);
    filePathAndNameLengthEdit = new QLineEdit();
    mainLayout->addWidget(filePathAndNameLengthEdit, 0, 3);

    mainLayout->addWidget(new QLabel("文件路径和文件名:"), 1, 0);
    filePathAndNameEdit = new QLineEdit();
    mainLayout->addWidget(filePathAndNameEdit, 1, 1, 1, 3);

    QPushButton *fileSelect = new QPushButton("选择传输文件");
    fileSelect->setIcon(QIcon(":/icon/file.png"));
    //mainLayout->addWidget(fileSelect, 1, 3);

    mainLayout->addWidget(new QLabel("数据格式标识符:"), 2, 0);
    dataFormatIdentifierEdit = new QLineEdit();
    mainLayout->addWidget(dataFormatIdentifierEdit, 2, 1);

    mainLayout->addWidget(new QLabel("文件大小参数长度:"), 2, 2);
    fileSizeParameterLengthEdit = new QLineEdit();
    mainLayout->addWidget(fileSizeParameterLengthEdit, 2, 3);

    mainLayout->addWidget(new QLabel("未压缩文件大小:"), 3, 0);
    fileSizeUnCompressedEdit = new QLineEdit();
    mainLayout->addWidget(fileSizeUnCompressedEdit, 3, 1);

    mainLayout->addWidget(new QLabel("压缩文件大小:"), 3, 2);
    fileSizeCompressedEdit = new QLineEdit();
    mainLayout->addWidget(fileSizeCompressedEdit, 3, 3);

    mainLayout->addWidget(new QLabel("MD5:"), 4, 0);
    QLineEdit *fileMd5Edit = new QLineEdit();
    mainLayout->addWidget(fileMd5Edit, 4, 1, 1, 2);

    QCheckBox *md5Check = new QCheckBox("计算文件MD5");
    mainLayout->addWidget(md5Check, 4, 3);

    mainLayout->addWidget(fileSelectWidget, 5, 0, 1, 4);

    connect(fileSelectWidget, &FileSelect::fileChange, this, [this, fileMd5Edit, md5Check](QString fileName){
        localFilePath = fileName;
        QFileInfo fileInfo(fileName);
        QFile file(fileName);

        fileTransferPath = fileInfo.path();
        switch (QMessageBox::information(0, tr("确认文件"), tr(QString("再次确认是否使用(%1)?").arg(fileName).toStdString().c_str()), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default: return ; break;
        }

        if (QFileInfo(servicetransferFileDir + fileInfo.fileName()).exists()) {
            switch (QMessageBox::information(0, tr("加载传输文件"), tr("文件已存在是否替换?"), tr("是"), tr("否"), 0, 1 )) {
              case 0: break;
              case 1: default:return;break;
            }
        }
        if (file.copy(servicetransferFileDir + fileInfo.fileName())) {
            localFilePath = fileInfo.fileName();
        }
        fileSizeCompressedEdit->setText(QString("%1").arg(fileInfo.size()));
        fileSizeUnCompressedEdit->setText(QString("%1").arg(fileInfo.size()));
        fileSizeParameterLengthEdit->setText("4");
        dataFormatIdentifierEdit->setText("0");
        filePathAndNameEdit->clear();
        filePathAndNameEdit->setText(fileInfo.fileName());
        filePathAndNameLengthEdit->setText(QString("%1").arg(filePathAndNameEdit->text().size()));

        if (md5Check->checkState() == Qt::Checked) {
            if (file.size() > 100 * 1024 * 1024) {
                switch (QMessageBox::information(0, tr("MD5计算"), tr("文件较大计算MD5耗时较长是否继续?"), tr("是"), tr("否"), 0, 1 )) {
                  case 0: break;
                  case 1: default: return ; break;
                }
            }
            QFile theFile(fileName);
            theFile.open(QIODevice::ReadOnly);
            QByteArray ba = QCryptographicHash::hash(theFile.readAll(), QCryptographicHash::Md5);
            theFile.close();
            fileMd5Edit->setText(ba.toHex().constData());
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(fileMd5Edit->text());
        }
    });

    connect(fileSelect, &QPushButton::clicked, this, [this, mainWidget, fileMd5Edit, md5Check]{
        QString fileName = QFileDialog::getOpenFileName(mainWidget, "选择文件", \
                             fileTransferPath.size() == 0 ? "C:\\Users\\Administrator\\Desktop" : fileTransferPath);
        if (fileName.size() == 0) { return ; }
        localFilePath = fileName;
        QFileInfo fileInfo(fileName);
        QFile file(fileName);

        fileTransferPath = fileInfo.path();
        switch (QMessageBox::information(0, tr("确认文件"), tr(QString("再次确认是否使用(%1)?").arg(fileName).toStdString().c_str()), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default: return ; break;
        }

        if (QFileInfo(servicetransferFileDir + fileInfo.fileName()).exists()) {
            switch (QMessageBox::information(mainWidget, tr("加载传输文件"), tr("文件已存在是否替换?"), tr("是"), tr("否"), 0, 1 )) {
              case 0: break;
              case 1: default:return;break;
            }
        }
        if (file.copy(servicetransferFileDir + fileInfo.fileName())) {
            localFilePath = fileInfo.fileName();
        }
        fileSizeCompressedEdit->setText(QString("%1").arg(fileInfo.size()));
        fileSizeUnCompressedEdit->setText(QString("%1").arg(fileInfo.size()));
        fileSizeParameterLengthEdit->setText("4");
        dataFormatIdentifierEdit->setText("0");
        filePathAndNameEdit->setText(fileInfo.fileName());
        filePathAndNameLengthEdit->setText(QString("%1").arg(filePathAndNameEdit->text().size()));

        if (md5Check->checkState() == Qt::Checked) {
            QFile theFile(fileName);
            theFile.open(QIODevice::ReadOnly);
            QByteArray ba = QCryptographicHash::hash(theFile.readAll(), QCryptographicHash::Md5);
            theFile.close();
            fileMd5Edit->setText(ba.toHex().constData());
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(fileMd5Edit->text());
        }
    });

    connect(filePathAndNameEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        Q_UNUSED(text);
        filePathAndNameLengthEdit->setText(QString("%1").arg(filePathAndNameEdit->text().size()));
        QString dataString = widgetConfigToString();
        serviceItem data = serviceItemDataDecode(dataString);
        QByteArray msg = getRequestFileTransferMessage(data);
        diagReqMsgEdit->setPlainText(msg.toHex(' '));
    });

    return mainWidget;
}

QWidget *UDS::securityAccessWidget()
{
    QWidget *mainWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QHBoxLayout *saConfigLayout = new QHBoxLayout();
    mainLayout->addLayout(saConfigLayout);
    mainLayout->addWidget(new QLabel(QString("<font color = red>※支持工具自定义, ZLG, CANoe dll文件格式</font>")));
    mainWidget->setLayout(mainLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    SecurityAccessComBox = new QComboBox();
    SecurityAccessComBox->addItem("Default");
    QLabel *selectLabel = new QLabel("27服务算法dll文件选择:");
    selectLabel->setMaximumWidth(130);
    saConfigLayout->addWidget(selectLabel);
    saConfigLayout->addWidget(SecurityAccessComBox);

    QDir dir(securityDllDir);
    if (!dir.exists()) {
        dir.mkdir(securityDllDir);
    }
    QFileInfoList list = dir.entryInfoList();
    dir.setNameFilters(QStringList() << "*.dll");
    foreach(QFileInfo fileinfo, list) {
        if (fileinfo.completeSuffix() == "dll") {
            SecurityAccessComBox->addItem(fileinfo.fileName().remove(".dll"));
        }
    }

    QPushButton *fileSelect = new QPushButton("加载算法文件");
    fileSelect->setMaximumWidth(100);
    fileSelect->setIcon(QIcon(":/icon/file.png"));
    saConfigLayout->addWidget(fileSelect);
    connect(fileSelect, &QPushButton::clicked, this, [this, mainWidget]{
        QString fileName = QFileDialog::getOpenFileName(mainWidget, "加载算法文件", "C:\\Users\\Administrator\\Desktop", tr("dll (*.dll)"));
        if (!fileName.size()) {
            return ;
        }

        QLibrary lib(fileName);
        if(!lib.load()) {
            qDebug() << __FUNCTION__ << "security_access_generate_key" << "load err";
            return ;
        }
        if (lib.resolve("security_access_generate_key") == nullptr &&\
            lib.resolve("ZLGKey") == nullptr && \
            lib.resolve("GenerateKeyEx") == nullptr) {
            lib.unload();
            qDebug() << __FUNCTION__ << "security_access_generate_key is null";
            QMessageBox::warning(mainWidget, tr("加载算法文件"), tr("dll算法文件格式不兼容，无法使用"));
            return ;
        }
        lib.unload();

        QFileInfo fileInfo(fileName);
        QString destFilePath = securityDllDir + fileInfo.fileName();
        if (QFileInfo(destFilePath).exists()) {
            switch (QMessageBox::information(mainWidget, tr("加载算法文件"), tr("文件已存在是否替换?"), tr("是"), tr("否"), 0, 1 )) {
              case 0: break;
              case 1: default:return;break;
            }
        }
        QFile::copy(fileName, destFilePath);

        SecurityAccessComBox->clear();
        SecurityAccessComBox->addItem("Default");
        QDir dir(securityDllDir);
        QFileInfoList list = dir.entryInfoList();
        dir.setNameFilters(QStringList() << "*.dll");
        foreach(QFileInfo fileinfo, list) {
            if (fileinfo.completeSuffix() == "dll") {
                SecurityAccessComBox->addItem(fileinfo.fileName().remove(".dll"));
                qDebug() << fileinfo.fileName();
            }
        }
    });


    return mainWidget;
}

QWidget *UDS::serviceProjectListWidget()
{
    QWidget *mainWidget = new QWidget();
    mainWidget->setStyleSheet(btnCommonStyle);

    projectListTimer = new QTimer(this);

    QVBoxLayout *projectNameLayout = new QVBoxLayout();
    mainWidget->setLayout(projectNameLayout);

    projectNameListComBox = new QComboBox();
    projectNameListComBox->setEditable(true);
    projectNameLayout->addWidget(projectNameListComBox);

    QHBoxLayout *sldBtnLayout = new QHBoxLayout();
    projectNameLayout->addLayout(sldBtnLayout);
    QPushButton *saveprojectNameListWidgetBtn = new QPushButton("保存");
    saveprojectNameListWidgetBtn->setIcon(QIcon(":/icon/save.png"));
    sldBtnLayout->addWidget(saveprojectNameListWidgetBtn);

    QPushButton *loadprojectNameListWidgetBtn = new QPushButton("加载");
    loadprojectNameListWidgetBtn->setIcon(QIcon(":/icon/load.png"));
    sldBtnLayout->addWidget(loadprojectNameListWidgetBtn);

    QPushButton *deleteprojectNameListWidgetBtn = new QPushButton("删除");
    deleteprojectNameListWidgetBtn->setIcon(QIcon(":/icon/delete.png"));
    sldBtnLayout->addWidget(deleteprojectNameListWidgetBtn);

    QDir dir(serviceProjectListConfigDir);
    if (!dir.exists()) {
        dir.mkdir(serviceProjectListConfigDir);
    }
    QFileInfoList list = dir.entryInfoList();
    dir.setNameFilters(QStringList() << "*.json");
    projectNameListComBox->clear();
    foreach(QFileInfo fileinfo, list) {
        if (fileinfo.completeSuffix() == "json") {
            projectNameListComBox->addItem(fileinfo.fileName());
        }
    }
    connect(saveprojectNameListWidgetBtn, &QPushButton::clicked, this, [this, saveprojectNameListWidgetBtn]{
        if (serviceActive()) {
            promptForAction(QString("<font color = red><测试中无法%1</font>").arg(saveprojectNameListWidgetBtn->text()));
            return ;
        }
        if (projectNameModel->rowCount() == 0) {
            promptForAction(QString("<font color = red>%1表格中无数据项</font>").arg(saveprojectNameListWidgetBtn->text()));
            return ;
        }
        QString fileName = projectNameListComBox->lineEdit()->text();
        if (fileName.size() == 0) {
            promptForAction(QString("<font color = red>%1失败条件文件名长度不能为0</font>").arg(saveprojectNameListWidgetBtn->text()));
            return ;
        }

        QFileInfo fileinfo(serviceProjectListConfigDir + "/" + fileName);
        if (fileinfo.isFile()) {
            switch (QMessageBox::information(0, tr("覆盖文件"), tr("是否覆盖同名文件?"), tr("是"), tr("否"), 0, 1 )) {
              case 0: break;
              case 1: default: return ; break;
            }
            fileName =  serviceProjectListConfigDir + "/" + fileName;
        }
        else {
            if (fileName.contains("json")) {
                promptForAction(QString("<font color = red>%1失败条件不满足(文件名不能包含“json”字符串)</font>")\
                                .arg(saveprojectNameListWidgetBtn->text()));
                return ;
            }
            fileName =  serviceProjectListConfigDir + "/" + fileName + ".json";
        }

        QFile file(fileName);
        file.open(QIODevice::ReadWrite);
        for (int nn = 0; nn < projectNameModel->rowCount(); nn++) {
            QJsonObject rootObj;
            if (projectNameModel->item(nn, 0)) {
                rootObj.insert("Project", projectNameModel->item(nn, 0)->text());
            }
            file.write(QJsonDocument(rootObj).toJson(QJsonDocument::Compact));
            file.write("\n");
        }
        file.close();
        projectNameListComBox->addItem(fileinfo.fileName());
        projectNameListComBox->setCurrentText(fileinfo.fileName());
        promptForAction(QString("<a style='color: rgb(30, 144, 255);' href=\"file:///%1\">保存成功:%2</a>")\
                        .arg(serviceProjectListConfigDir).arg(fileName));
    });

    connect(loadprojectNameListWidgetBtn, &QPushButton::clicked, this, [this]{
        if (serviceActive()) {
            promptForAction(QString("<font color = red>测试中无法加载</font>"));
            return ;
        }
        if (projectNameListComBox->lineEdit()->text().size() == 0) {
            return ;
        }
        projectNameModel->removeRows(0, projectNameModel->rowCount());
        QString fileName =  serviceProjectListConfigDir + projectNameListComBox->lineEdit()->text();
        QFileInfo fileInfo(fileName);
        if (fileInfo.exists()) {
            QFile file(fileName);
            file.open(QIODevice::ReadOnly);
            QByteArray linedata;
            do {
                linedata.clear();
                linedata = file.readLine();
                if (linedata.size()) {
                    QString config = QString(linedata).toUtf8().data();
                    QJsonParseError err;

                    QJsonObject ProjectObj = QJsonDocument::fromJson(config.toUtf8(), &err).object();
                    if (err.error)
                        return ;

                    if (ProjectObj.contains("Project")) {
                        QList<QStandardItem *> items;
                        items.append(new QStandardItem(ProjectObj["Project"].toString()));
                        projectNameModel->appendRow(items);
                    }
                }
            } while (linedata.size() > 0);
            file.close();
            promptForAction(QString("<font color = green>加载成功</font>"));
        }
    });

    connect(deleteprojectNameListWidgetBtn, &QPushButton::clicked, this, [this]{
        if (serviceActive()) {
            promptForAction(QString("<font color = red>测试中无法删除</font>"));
            return ;
        }

        if (projectNameListComBox->lineEdit()->text().size() == 0) {
            return ;
        }

        switch (QMessageBox::information(0, tr("删除"), tr("确认删除?"), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default: return ; break;
        }
        QString fileName =  serviceProjectListConfigDir + projectNameListComBox->lineEdit()->text();
        QFileInfo fileInfo(fileName);
        if (fileInfo.exists()) {
            QFile file(fileName);
            file.remove();
            projectNameListComBox->removeItem(projectNameListComBox->currentIndex());
            promptForAction(QString("<font color = green>删除成功</font>"));
        }
    });

    QHBoxLayout *paBtnLayout = new QHBoxLayout();
    projectNameLayout->addLayout(paBtnLayout);
    projectNameComBox = new QComboBox();
    paBtnLayout->addWidget(projectNameComBox);

    QPushButton *addBtn = new QPushButton("添加");
    addBtn->setIcon(QIcon(":/icon/addList.png"));
    addBtn->setMaximumWidth(50);
    paBtnLayout->addWidget(addBtn);

    QHBoxLayout *ssBtnLayout = new QHBoxLayout();
    projectNameLayout->addLayout(ssBtnLayout);
    QPushButton *startBtn = new QPushButton("开始");
    startBtn->setIcon(QIcon(":/icon/testing.png"));
    ssBtnLayout->addWidget(startBtn);

    QPushButton *stopBtn = new QPushButton("停止");
    stopBtn->setIcon(QIcon(":/icon/stop.png"));
    ssBtnLayout->addWidget(stopBtn);

    QPushButton *clearBtn = new QPushButton("清除");
    clearBtn->setIcon(QIcon(":/icon/clear.png"));
    ssBtnLayout->addWidget(clearBtn);

    projectNameModel = new QStandardItemModel();
    projectNameTable = new QTableView();
    projectNameLayout->addWidget(projectNameTable);
    projectNameTable->setModel(projectNameModel);
    projectNameTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    projectNameTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    projectNameTable->setAlternatingRowColors(true);
    projectNameTable->horizontalHeader()->hide();
    projectNameTable->setContextMenuPolicy(Qt::CustomContextMenu);
    projectNameWidgetMenu = new QMenu();
    QAction *projectSetDelAction = new QAction("删除此项");
    projectSetDelAction->setIcon(QIcon(":/icon/delete.png"));
    projectNameWidgetMenu->addAction(projectSetDelAction);

    QAction *projectLoadAction = new QAction("加载此项");
    projectLoadAction->setIcon(QIcon(":/icon/load.png"));
    projectNameWidgetMenu->addAction(projectLoadAction);

    QAction *projectSetUpAction = new QAction("上移一行");
    projectSetUpAction->setIcon(QIcon(":/icon/up.png"));
    projectNameWidgetMenu->addAction(projectSetUpAction);

    QAction *projectSetDownAction = new QAction("下移一行");
    projectSetDownAction->setIcon(QIcon(":/icon/down.png"));
    projectNameWidgetMenu->addAction(projectSetDownAction);

    QAction *projectSetMoveAction = new QAction("移动至行");
    projectSetMoveAction->setIcon(QIcon(":/icon/move.png"));
    projectNameWidgetMenu->addAction(projectSetMoveAction);

    QDialog *projectSetMoveDiag = new QDialog();
    projectSetMoveDiag->setAttribute(Qt::WA_QuitOnClose, false);
    projectSetMoveDiag->setWindowTitle("移动至行");
    QHBoxLayout *projectSetMoveDiagLayout = new QHBoxLayout();
    projectSetMoveDiag->setLayout(projectSetMoveDiagLayout);
    QLabel *projectSetMoveHint = new QLabel("移动到");
    projectSetMoveDiagLayout->addWidget(projectSetMoveHint);
    QLineEdit *projectSetMoveDiagEdit = new QLineEdit();
    projectSetMoveDiagEdit->setMaximumWidth(40);
    projectSetMoveDiagEdit->setValidator(new QIntValidator(0, 100000, projectSetMoveDiagEdit));
    projectSetMoveDiagLayout->addWidget(projectSetMoveDiagEdit);
    projectSetMoveDiagLayout->addWidget(new QLabel("行"));
    QPushButton *projectSetMoveDiagConfirmBtn = new QPushButton("确定");
    projectSetMoveDiagLayout->addWidget(projectSetMoveDiagConfirmBtn);
    connect(projectSetMoveDiagConfirmBtn, &QPushButton::clicked, this, \
            [this, projectSetMoveDiag, projectSetMoveDiagEdit] {
        projectSetMoveDiag->hide();
        int newRow = projectSetMoveDiagEdit->text().toInt();
        qDebug() << "newRow:" << newRow << " " << projectNameModel->rowCount();
        projectSetMoveItem(projectNameTable->selectionModel()->currentIndex().row(), newRow - 1);

    });

    connect(projectSetDelAction, &QAction::triggered, this, [this]{
        switch (QMessageBox::information(0, tr("删除"), tr("确认删除?"), tr("是"), tr("否"), 0, 1 )) {
          case 0:break;
          case 1:default: return ; break;
        }
        if (projectSetSelectRow < 0 || projectSetSelectRow >= projectNameModel->rowCount()) {
            return ;
        }
        projectNameModel->removeRow(projectSetSelectRow);
    });

    connect(projectLoadAction, &QAction::triggered, this, [this]{
        if (projectSetSelectRow < 0 || projectSetSelectRow >= projectNameModel->rowCount()) {
            return ;
        }
        QStandardItem *item = projectNameModel->item(projectSetSelectRow, 0);
        if (item) {
            readServiceSet(serviceSetProjectDir + item->text());
        }
    });

    connect(projectSetUpAction, &QAction::triggered, this, [this]{
        projectSetMoveItem(projectNameTable->selectionModel()->currentIndex().row(), \
                           projectNameTable->selectionModel()->currentIndex().row() - 1);
    });

    connect(projectSetDownAction, &QAction::triggered, this, [this]{
        projectSetMoveItem(projectNameTable->selectionModel()->currentIndex().row(), \
                           projectNameTable->selectionModel()->currentIndex().row() + 1);
    });

    connect(projectSetMoveAction, &QAction::triggered, this, [this, projectSetMoveDiag, projectSetMoveHint]{
        projectSetMoveDiag->hide();
        projectSetMoveHint->setText(QString("第%1移动到").arg(projectNameTable->selectionModel()->currentIndex().row() + 1));
        projectSetMoveDiag->show();
    });

    connect(projectNameTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(projectSetContextMenuSlot(QPoint)));

    mainWidget->setMaximumWidth(350);

    connect(addBtn, &QPushButton::clicked, this, [this](){
        if (projectNameComBox->currentText().size()) {
            QList<QStandardItem *> items;
            items.append(new QStandardItem(projectNameComBox->currentText()));
            projectNameModel->appendRow(items);
        }
    });

    connect(startBtn, &QPushButton::clicked, this, [this, startBtn, stopBtn, addBtn, clearBtn](){
        projectListIndex = -1;
        abortServiceTask();
        if (projectNameModel->rowCount() > 0) {
            projectListIndex = 0;
        }
        if (projectListIndex >= 0) {
            projectListTimer->setInterval(10);
            projectListTimer->start();
            startBtn->setEnabled(false);
            clearBtn->setEnabled(false);
            stopBtn->setEnabled(true);
            addBtn->setEnabled(false);
            for (int index = 0; index < projectNameModel->rowCount(); index++) {
                if (index % 2) {
                    setTableItemColor(projectNameModel, index, "#F5F5F5");
                } else {
                    setTableItemColor(projectNameModel, index, "#FFFFFF");
                }
            }
            generateTestResultFileName();
            cycleNumberEdit->setText("0");
        }
    });

    connect(clearBtn, &QPushButton::clicked, this, [this](){
        switch (QMessageBox::information(0, tr("删除"), tr("确认清除列表?"), tr("是"), tr("否"), 0, 1 )) {
          case 0: break;
          case 1: default: return ; break;
        }
        projectNameModel->removeRows(0, projectNameModel->rowCount());
    });

    connect(stopBtn, &QPushButton::clicked, this, [this, startBtn, stopBtn, addBtn, clearBtn](){
        projectListIndex = -1;
        activeServiceProjectFlicker.isvalid = false;
        abortServiceTask();
        projectListTimer->stop();
        startBtn->setEnabled(true);
        clearBtn->setEnabled(true);
        stopBtn->setEnabled(false);
        addBtn->setEnabled(true);
        testResultCheckBox->setCheckState(Qt::Unchecked); /* 关闭结果记录，防止误开导致巨大的日志文件 */
        for (int index = 0; index < projectNameModel->rowCount(); index++) {
            if (index % 2) {
                setTableItemColor(projectNameModel, index, "#F5F5F5");
            } else {
                setTableItemColor(projectNameModel, index, "#FFFFFF");
            }
        }
    });

    connect(projectListTimer, &QTimer::timeout, this, [this, startBtn, stopBtn, addBtn, clearBtn] {
        if (!serviceActive() && (projectListIndex < 0 || \
            projectListIndex >= projectNameModel->rowCount())) {
            projectListIndex = -1;
            projectListTimer->stop();
            startBtn->setEnabled(true);
            clearBtn->setEnabled(true);
            stopBtn->setEnabled(false);
            addBtn->setEnabled(true);
            activeServiceProjectFlicker.isvalid = false;
            testResultCheckBox->setCheckState(Qt::Unchecked); /* 关闭结果记录，防止误开导致巨大的日志文件 */
        } else {
            if (!serviceActive()) {
                QStandardItem *item = projectNameModel->item(projectListIndex, 0);
                if (item) {
                    projectNameTable->scrollTo(projectNameModel->index(projectListIndex, 0));
                    QStandardItem *previtem = projectNameModel->item(projectListIndex - 1, 0);
                    if (!previtem || previtem->text() != item->text()) {
                        readServiceSet(serviceSetProjectDir + item->text());
                    }
                    QList<QStandardItem *> itemList;
                    itemList.append(item);
                    setActiveServiceProjectFlicker(itemList);
                    sendListBtn->click();
                }
                //setTableItemColor(projectNameModel, projectListIndex, "#00B2EE");
                projectNameTable->scrollTo(projectNameModel->index(projectListIndex, 0));
                projectListIndex++;
            }
        }
    });

    return mainWidget;
}

bool UDS::saveCurrServiceSet(QString filename)
{
    QFile file(filename);

    if (file.exists()) {
        file.remove();
    }

    file.open(QIODevice::ReadWrite);
    for (int row = 0; row < serviceSetModel->rowCount(); row++) {
        serviceItem data;
        getServiceItemData(row, data);
        file.write(serviceItemDataEncodeUnformat(data).toUtf8() + "\n");
    }
    file.close();

    return true;
}

bool UDS::addServiceItem(serviceItem &data)
{
    serviceSetBackTimer->start();
    serviceSetModel->appendRow(0);

    QStandardItem *msgItem = new QStandardItem(QString(data.requestMsg.toHex(' ')));
    msgItem->setToolTip(serviceItemDataEncode(data));
    serviceSetModel->setItem(serviceSetModel->rowCount() - 1, udsViewRequestColumn, msgItem);
    serviceSetModel->setItem(serviceSetModel->rowCount() - 1, udsViewSupressColumn, new QStandardItem(QString("%1").arg(data.isspuress)));
    serviceSetModel->setItem(serviceSetModel->rowCount() - 1, udsViewTimeoutColumn, new QStandardItem(QString("%1").arg(data.timeout)));
    serviceSetModel->setItem(serviceSetModel->rowCount() - 1, udsViewDelayColumn, new QStandardItem(QString("%1").arg(data.delay)));
    serviceSetModel->setItem(serviceSetModel->rowCount() - 1, udsViewElapsedTimeColumn, new QStandardItem(""));
    serviceSetModel->setItem(serviceSetModel->rowCount() - 1, udsViewResponseColumn, new QStandardItem(""));
    serviceSetModel->setItem(serviceSetModel->rowCount() - 1, udsViewExpectResponseColumn, new QStandardItem(""));
    serviceSetModel->setItem(serviceSetModel->rowCount() - 1, udsViewRemarkColumn, new QStandardItem(data.desc));
    serviceSetModel->setItem(serviceSetModel->rowCount() - 1, udsViewResultColumn, new QStandardItem(serviceItemStateColor[serviceItemNotTest]));

    setServiceItemOperationControl(serviceSetModel->rowCount() - 1, data);

    serviceItemStatis[serviceItemAll] = serviceSetModel->rowCount();
    serviceItemStatisLabel[serviceItemAll]->setText(QString("总共:%1").arg(serviceItemStatis[serviceItemAll]));
    serviceTestProgress->setMaximum(serviceItemStatis[serviceItemAll]);

    return true;
}

bool UDS::setServiceItemOperationControl(int row, serviceItem &data)
{
    /* 加入抑制响应选择框 */
    QCheckBox *suppressCheck = new QCheckBox();
    serviceSetView->setIndexWidget(serviceSetModel->index(row, udsViewSupressColumn) , suppressCheck);
    if (data.isspuress) {
        suppressCheck->click();
    }
    suppressCheck->setEnabled(false);

    /* 加入使能选择框 */
    QCheckBox *enableItem = new QCheckBox();
    enableItem->setCheckState(Qt::Checked);
    serviceSetView->setIndexWidget(serviceSetModel->index(row, udsViewItemEnable) , enableItem);

    QPushButton *modifyBtn = new QPushButton("配置");
    serviceSetView->setIndexWidget(serviceSetModel->index(row, udsViewConfigBtnColumn) , modifyBtn);
    connect(modifyBtn, &QPushButton::clicked, this, [this] {
        serviceSetChangeItem(serviceSetView->selectionModel()->currentIndex().row());
    });
    modifyBtn->setStyleSheet("QPushButton{background-color:#87CEEB; color: black; \
                                        border-radius: 2px; \
                                        border: 1px groove gray;}"\
                                        "QPushButton:hover{background-color:white; color: black;}"\
                                        "QPushButton:pressed{background-color:#48D1CC;\
                                        border-style: inset; }");


    QPushButton *sendBtn = new QPushButton("发送");
    serviceSetView->setIndexWidget(serviceSetModel->index(row, udsViewSendBtnColumn) , sendBtn);
    connect(sendBtn, &QPushButton::clicked, this, [this] {
        if (serviceActive()) return ;

        int index = serviceSetView->selectionModel()->currentIndex().row();
        if (adbDiagCheckBox->checkState() == Qt::Checked) {
            sendServiceRequest(index, adbDiagChannelName);
        }
        else if (canDiagCheckBox->checkState() == Qt::Checked){
            sendServiceRequest(index, can->getCurrChannel());
        }
        else {
            sendServiceRequest(index, doipServerListBox->currentText());
        }
        manualSendRowIndex = index;
    });
    sendBtn->setStyleSheet("QPushButton{background-color:#43CD80; color: black; \
                                        border-radius: 2px; \
                                        border: 1px groove gray;}"\
                                        "QPushButton:hover{background-color:white; color: black;}"\
                                        "QPushButton:pressed{background-color:#00EE76;\
                                        border-style: inset; }");

    QPushButton *delBtn = new QPushButton("删除");
    serviceSetView->setIndexWidget(serviceSetModel->index(row, udsViewDelBtnColumn) , delBtn);
    connect(delBtn, &QPushButton::clicked, this, [this] {
        switch (QMessageBox::information(0, tr("删除"), tr("确认删除?"), tr("是"), tr("否"), 0, 1 )) {
          case 0:break;
          case 1:default: return ; break;
        }
        serviceSetDelItem(serviceSetView->selectionModel()->currentIndex().row());
    });
    delBtn->setStyleSheet("QPushButton{background-color:#FF0000; color: black; \
                                        border-radius: 2px; \
                                        border: 1px groove gray;}"\
                                        "QPushButton:hover{background-color:white; color: black;}"\
                                        "QPushButton:pressed{background-color:#CD2626;\
                                        border-style: inset; }");

    QPushButton *upBtn = new QPushButton("上移");
    serviceSetView->setIndexWidget(serviceSetModel->index(row, udsViewUpBtnColumn) , upBtn);
    connect(upBtn, &QPushButton::clicked, this, [this] {
        int selectRow = serviceSetView->selectionModel()->currentIndex().row();
        if (selectRow == 0) {
            return ;
        }
        serviceSetMoveItem(selectRow, selectRow - 1);
    });
    upBtn->setStyleSheet("QPushButton{background-color:#BBFFFF; color: black; \
                                        border-radius: 2px; \
                                        border: 1px groove gray;}"\
                                        "QPushButton:hover{background-color:white; color: black;}"\
                                        "QPushButton:pressed{background-color:#98F5FF;\
                                        border-style: inset; }");

    QPushButton *downBtn = new QPushButton("下移");
    serviceSetView->setIndexWidget(serviceSetModel->index(row, udsViewDownBtnColumn) , downBtn);
    connect(downBtn, &QPushButton::clicked, this, [this] {
        int selectRow = serviceSetView->selectionModel()->currentIndex().row();
        if (selectRow == serviceSetModel->rowCount() - 1) {
            return ;
        }
        serviceSetMoveItem(selectRow, selectRow + 1);
    });
    downBtn->setStyleSheet("QPushButton{background-color:#98F5FF; color: black; \
                                        border-radius: 2px; \
                                        border: 1px groove gray;}"\
                                        "QPushButton:hover{background-color:white; color: black;}"\
                                        "QPushButton:pressed{background-color:#BBFFFF;\
                                        border-style: inset; }");

    return true;
}

bool UDS::modifyServiceItem(int row, serviceItem &data)
{
    serviceSetBackTimer->start();
    QStandardItem *requestItem = serviceSetModel->item(row, udsViewRequestColumn);
    if (requestItem) {
        requestItem->setText(QString(data.requestMsg.toHex(' ')));
        requestItem->setToolTip(serviceItemDataEncode(data));
    }
    QCheckBox* suppressCheck = qobject_cast<QCheckBox*>(serviceSetView->indexWidget(serviceSetModel->index(row, udsViewSupressColumn)));
    if (suppressCheck) {
        suppressCheck->setEnabled(true);
        if (data.isspuress) {
            suppressCheck->setCheckState(Qt::Checked);
        }
        else {
            suppressCheck->setCheckState(Qt::Unchecked);
        }
        suppressCheck->setEnabled(false);
    }
    QStandardItem *timeoutItem = serviceSetModel->item(row, udsViewTimeoutColumn);
    if (timeoutItem) {
        timeoutItem->setText(QString("%1").arg(data.timeout));
    }
    QStandardItem *delayItem = serviceSetModel->item(row, udsViewDelayColumn);
    if (delayItem) {
        delayItem->setText(QString("%1").arg(data.delay));
    }

    QStandardItem *remarkItem = serviceSetModel->item(row, udsViewRemarkColumn);
    if (remarkItem) {
        remarkItem->setText(QString("%1").arg(data.desc));
    }

    return true;
}

bool UDS::getServiceItemData(int row, serviceItem &data)
{
    QStandardItem *msgItem = serviceSetModel->item(row, udsViewRequestColumn);
    if (msgItem) {
        QString toolTip = msgItem->toolTip();
        data = serviceItemDataDecode(toolTip);
        return true;
    }

    return false;
}

QJsonDocument UDS::serviceItemDataToJson(serviceItem &data)
{
    QJsonObject rootObj;
    QJsonObject requestObj;
    QJsonObject responseObj;

    requestObj.insert("Suppress", data.isspuress);
    requestObj.insert("Service", QString::number(data.SID, 16));
    if (data.SUB > 0) {
        requestObj.insert("ServiceSub", QString::number(data.SUB, 16));
    }
    if (data.did > 0) {
        requestObj.insert("DataIdentifier", QString::number(data.did, 16));
    }
    requestObj.insert("Timeout", QString::number(data.timeout, 10));
    requestObj.insert("Delay", QString::number(data.delay, 10));
    requestObj.insert("Payload", QString(data.requestMsg.toHex(' ')));
    requestObj.insert("Describe", data.desc);
    requestObj.insert("TA", QString::number(data.ta, 16));
    requestObj.insert("TATYPE", data.tatype);

    if (data.SID == RequestFileTransfer) {
        requestObj.insert("modeOfOperation", modeOfOperationDescMap[data.modeOfOperation]);
        requestObj.insert("filePathAndNameLength", QString::number(data.filePathAndNameLength, 10));
        requestObj.insert("filePathAndName", data.filePathAndName);
        requestObj.insert("dataFormatIdentifier", QString::number(data.dataFormatIdentifier, 10));
        requestObj.insert("fileSizeParameterLength", QString::number(data.fileSizeParameterLength, 10));
        requestObj.insert("fileSizeUnCompressed", QString::number(data.fileSizeUnCompressed, 10));
        requestObj.insert("fileSizeCompressed", QString::number(data.fileSizeCompressed, 10));
        requestObj.insert("localFilePath", data.localFilePath);
    }
    else if (data.SID == SecurityAccess) {
        requestObj.insert("securityAccess", data.securityAccess);
    }

    responseObj.insert("ExpectResponse", data.expectResponse);
    responseObj.insert("ExpectMessage", QString(data.expectResponseRule));

    responseObj.insert("UDSFinishMaxNumber", QString::number(data.UDSFinishMaxNumber, 10));
    responseObj.insert("UDSFinishCondition", data.UDSFinishCondition);
    responseObj.insert("UDSFinishData", data.UDSFinishData);

    rootObj.insert("Request", requestObj);
    rootObj.insert("Response", responseObj);
    QJsonDocument doc(rootObj);
    return doc;
}

QString UDS::serviceItemDataEncode(serviceItem &data)
{
    QByteArray json= serviceItemDataToJson(data).toJson();
    return QString(json).toUtf8().data();
}

QString UDS::serviceItemDataEncodeUnformat(serviceItem &data)
{
    QByteArray json= serviceItemDataToJson(data).toJson(QJsonDocument::Compact);
    return QString(json).toUtf8().data();
}

QByteArray UDS::getRequestFileTransferMessage(serviceItem &data)
{
    QByteArray msg;

    if (data.SID == RequestFileTransfer) {
        msg.append(RequestFileTransfer);
        msg.append(data.modeOfOperation);
        msg.append(data.filePathAndNameLength >> 8 & 0xff);
        msg.append(data.filePathAndNameLength & 0xff);
        msg.append(data.filePathAndName);
        msg.append(data.dataFormatIdentifier);
        msg.append(data.fileSizeParameterLength);
        for (int n = data.fileSizeParameterLength - 1; n >= 0; n--) {
            msg.append((data.fileSizeUnCompressed >> (8 * n)) & 0xff);
        }
        for (int n = data.fileSizeParameterLength - 1; n >= 0; n--) {
            msg.append((data.fileSizeCompressed >> (8 * n)) & 0xff);
        }
    }

    return msg;
}

serviceItem UDS::serviceItemDataDecode(QString &config)
{
    serviceItem data;
    QJsonParseError err;
    QJsonObject requestObj;
    QJsonObject responseObj;

    QJsonObject UDSConfigObj = QJsonDocument::fromJson(config.toUtf8(), &err).object();
    if (err.error)
        return data;
    requestObj = UDSConfigObj["Request"].toObject();

    if (requestObj.contains("Suppress")) {
        data.isspuress = requestObj["Suppress"].toBool();
    } else {
        data.isspuress = false;
    }

    if (requestObj.contains("ServiceSub")) {
        data.SUB = requestObj["ServiceSub"].toString().toInt(nullptr, 16);
    } else {
        data.SUB = 0;
    }

    if (requestObj.contains("Service")) {
        data.SID = requestObj["Service"].toString().toInt(nullptr, 16);
    } else {
        data.SID = 0;
    }

    if (requestObj.contains("DataIdentifier")) {
        data.did = requestObj["DataIdentifier"].toString().toInt(nullptr, 16);
    } else {
        data.did = 0;
    }

    if (requestObj.contains("Timeout")) {
        data.timeout = requestObj["Timeout"].toString().toInt(nullptr, 10);
    } else {
        data.timeout = 0;
    }

    if (requestObj.contains("Delay")) {
        data.delay = requestObj["Delay"].toString().toInt(nullptr, 10);
    } else {
        data.delay = 0;
    }

    if (requestObj.contains("Payload")) {
        data.requestMsg = QByteArray::fromHex(requestObj["Payload"].toString().remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
    } else {
        data.requestMsg.clear();
    }

    if (requestObj.contains("Describe")) {
        data.desc = (requestObj["Describe"].toString());
    } else {
        data.desc.clear();
    }

    if (requestObj.contains("TA")) {
        data.ta = requestObj["TA"].toString().toInt(nullptr, 16);
    } else {
        data.ta = 0;
    }

    if (requestObj.contains("TATYPE")) {
        data.tatype = requestObj["TATYPE"].toString();
    } else {
        data.tatype = "physical address";
    }

    if (data.SID == 0x38) {
        data.modeOfOperation = modeOfOperationMap[requestObj["modeOfOperation"].toString()];
        data.filePathAndNameLength = requestObj["filePathAndNameLength"].toString().toInt();
        data.filePathAndName = requestObj["filePathAndName"].toString();
        data.dataFormatIdentifier = requestObj["dataFormatIdentifier"].toString().toInt();
        data.fileSizeParameterLength = requestObj["fileSizeParameterLength"].toString().toInt();
        data.fileSizeUnCompressed = requestObj["fileSizeUnCompressed"].toString().toInt();
        data.fileSizeCompressed = requestObj["fileSizeUnCompressed"].toString().toInt();
        localFilePath = data.localFilePath = requestObj["localFilePath"].toString();
    } else if (data.SID == 0x27) {
        data.securityAccess = requestObj["securityAccess"].toString();
    }

    responseObj = UDSConfigObj["Response"].toObject();
    if (responseObj.contains("ExpectMessage")) {
        data.expectResponseRule = responseObj["ExpectMessage"].toString();
    } else {
        data.expectResponseRule.clear();
    }

    if (responseObj.contains("ExpectResponse")) {
        data.expectResponse = (responseObj["ExpectResponse"].toString());
    } else {
        data.expectResponse.clear();
    }

    if (responseObj.contains("UDSFinishMaxNumber")) {
        data.UDSFinishMaxNumber = responseObj["UDSFinishMaxNumber"].toString().toInt();
    } else {
        data.UDSFinishMaxNumber = 0;
    }

    if (responseObj.contains("UDSFinishCondition")) {
        data.UDSFinishCondition = responseObj["UDSFinishCondition"].toString();
    } else {
        data.UDSFinishCondition.clear();
    }

    if (responseObj.contains("UDSFinishData")) {
        data.UDSFinishData = responseObj["UDSFinishData"].toString();
    } else {
        data.UDSFinishData.clear();
    }

    return data;
}



QByteArray UDS::serviceRequestEncode()
{
    QByteArray msg;

    msg.append(serviceEidt->text().toUInt(0, 16));
    if (serviceSubEidt->text().size()) {
        if (serviceSuppressCheck->checkState() == Qt::Checked) {
            msg.append(serviceSubEidt->text().toUInt(0, 16) + 0x80);
        }
        else {
            msg.append(serviceSubEidt->text().toUInt(0, 16));
        }
    }
    if (serviceDidEidt->text().size()) {
        msg.append(serviceDidEidt->text().toInt(nullptr, 16) >> 8 & 0xff);
        msg.append(serviceDidEidt->text().toInt(nullptr, 16) & 0xff);
    }

    return msg;
}

QString UDS::widgetConfigToString()
{
    QJsonObject rootObj;
    QJsonObject requestObj;
    QJsonObject responseObj;

    requestObj.insert("Suppress", serviceSuppressCheck->checkState() == Qt::Checked ? true : false);
    requestObj.insert("Service", serviceEidt->text());
    if (serviceSubEidt->text().size()) {
        requestObj.insert("ServiceSub", serviceSubEidt->text());
    }
    if (serviceDidEidt->text().size()) {
        requestObj.insert("DataIdentifier", serviceDidEidt->text());
    }
    requestObj.insert("Timeout", serviceTimeoutEdit->text());
    requestObj.insert("Delay", serviceDelayEdit->text());
    requestObj.insert("Payload", diagReqMsgEdit->toPlainText());
    if (serviceDesc->text().size() == 0) {
        serviceDesc->setText(serviceListBox->currentText() + serviceSubListBox->currentText());
    }
    requestObj.insert("Describe", serviceDesc->text());
    requestObj.insert("TA", serviceTaEdit->text());
    requestObj.insert("TATYPE", serviceAddrType->currentText());

    if (serviceListBox->currentText() == serviceDescMap[0x38]) {
        requestObj.insert("modeOfOperation", modeOfOperationComBox->currentText());
        requestObj.insert("filePathAndNameLength", filePathAndNameLengthEdit->text());
        requestObj.insert("filePathAndName", filePathAndNameEdit->text());
        requestObj.insert("dataFormatIdentifier", dataFormatIdentifierEdit->text());
        requestObj.insert("fileSizeParameterLength", fileSizeParameterLengthEdit->text());
        requestObj.insert("fileSizeUnCompressed", fileSizeUnCompressedEdit->text());
        requestObj.insert("fileSizeCompressed", fileSizeCompressedEdit->text());
        requestObj.insert("localFilePath", localFilePath);
    }
    else if (serviceListBox->currentText() == serviceDescMap[0x27]) {
        requestObj.insert("securityAccess", SecurityAccessComBox->currentText());
    }

    responseObj.insert("ExpectResponse", serviceExpectRespBox->currentText());
    responseObj.insert("ExpectMessage", serviceRespPlainText->toPlainText());
    responseObj.insert("UDSFinishCondition", UDSConditionRuleComboBox->currentText());
    responseObj.insert("UDSFinishMaxNumber", UDSFinishRuleEdit->text());
    responseObj.insert("UDSFinishData", UDSFinishPlainText->toPlainText());

    rootObj.insert("Request", requestObj);
    rootObj.insert("Response", responseObj);
    QJsonDocument doc(rootObj);
    QByteArray json= doc.toJson();
    qDebug()<< QString(json).toUtf8().data();
    return QString(json).toUtf8().data();
}

bool UDS::stringToWidgetConfig(QString &config)
{
    QJsonParseError err;
    QJsonObject requestObj;
    QJsonObject responseObj;

    QJsonObject UDSConfigObj = QJsonDocument::fromJson(config.toUtf8(), &err).object();
    if (err.error)
        return false;
    requestObj = UDSConfigObj["Request"].toObject();
    serviceSuppressCheck->setCheckState((requestObj["Suppress"].toBool() == true ?  Qt::Checked : Qt::Unchecked));
    if (requestObj.contains("ServiceSub")) {
        serviceSubEidt->setText(requestObj["ServiceSub"].toString());
    }
    serviceEidt->setText(requestObj["Service"].toString());
    if (requestObj.contains("DataIdentifier")) {
        serviceDidEidt->setText(requestObj["DataIdentifier"].toString());
    }
    serviceTimeoutEdit->setText(requestObj["Timeout"].toString());
    serviceDelayEdit->setText(requestObj["Delay"].toString());
    diagReqMsgEdit->setPlainText(requestObj["Payload"].toString());
    serviceDesc->setText(requestObj["Describe"].toString());
    serviceTaEdit->setText(requestObj["TA"].toString());
    serviceAddrType->setCurrentText(requestObj["TATYPE"].toString());

    if (serviceListBox->currentText() == serviceDescMap[0x38]) {
        modeOfOperationComBox->setCurrentText(requestObj["modeOfOperation"].toString());
        filePathAndNameLengthEdit->setText(requestObj["filePathAndNameLength"].toString());
        filePathAndNameEdit->setText(requestObj["filePathAndName"].toString());
        dataFormatIdentifierEdit->setText(requestObj["dataFormatIdentifier"].toString());
        fileSizeParameterLengthEdit->setText(requestObj["fileSizeParameterLength"].toString());
        fileSizeUnCompressedEdit->setText(requestObj["fileSizeUnCompressed"].toString());
        fileSizeCompressedEdit->setText(requestObj["fileSizeCompressed"].toString());
        localFilePath = requestObj["localFilePath"].toString();
    }
    else if (serviceListBox->currentText() == serviceDescMap[0x27]) {
        SecurityAccessComBox->setCurrentText(requestObj["securityAccess"].toString());
    }

    responseObj = UDSConfigObj["Response"].toObject();
    serviceExpectRespBox->setCurrentText(responseObj["ExpectResponse"].toString());
    serviceRespPlainText->setPlainText(responseObj["ExpectMessage"].toString());
    UDSConditionRuleComboBox->setCurrentText(responseObj["UDSFinishCondition"].toString());
    UDSFinishRuleEdit->setText(responseObj["UDSFinishMaxNumber"].toString());
    UDSFinishPlainText->setPlainText(responseObj["UDSFinishData"].toString());

    return true;
}

void UDS::setTableItemColor(QStandardItemModel *model, int index, QColor &color)
{
    for (int nn = 0; nn < model->columnCount(); nn++) {
        QStandardItem *item = model->item(index, nn);
        if (item) {
            item->setBackground(QBrush(color));
        }
    }
}

void UDS::setTableItemColor(QStandardItemModel *model, int index, const char *color)
{
    for (int nn = 0; nn < model->columnCount(); nn++) {
        QStandardItem *item = model->item(index, nn);
        if (item) {
            item->setBackground(QBrush(QColor(color)));
        }
    }
}

bool UDS::serviceActive()
{
    return serviceActiveTask.indexRow >= 0 && serviceActiveTask.indexRow < serviceSetModel->rowCount();
}

void UDS::serviceRequestHandler(serviceItem &data)
{
    if (data.SID == SecurityAccess && data.SUB % 2 == 0 && data.requestMsg.size() == 2) {
        /* 安全会话服务没有填充key的时候由算法生成key填充 */
        data.requestMsg.append(securityAccessKey);
        qDebug() << "data.msg" << data.requestMsg.toHex(' ');
    }
    else if (data.SID == TransferData && data.requestMsg.size() == 1 && transferFile.isOpen()) {
        /* 文件传输服务没有填充数据的话，自动根据配置打开文件填充数据 */
        QByteArray fileData = transferFile.read(maxNumberOfBlockLength - 2);

        data.requestMsg.append(fileTransferCount);
        data.requestMsg.append(fileData);
    }

    if (data.SID != TransferData) {
        promptActionProgress->hide();
    }
}

bool UDS::sendServiceRequest(int index, QString channelName)
{
    if (channelName.size() == 0) {
        setTableItemColor(serviceSetModel, index, serviceItemStateColor[serviceItemTestWarn]);
        return false;
    };

    int sendSuccess = false;
    serviceItem data;

    /* 判断是否固定周期发送3E服务 */
    if (cycle3eTimer->isActive()) {
        if (!(fixCycle3eCheckBox->checkState() == Qt::Checked)) {
            if (cycle3eEdit->text().size() > 0) {
                cycle3eTimer->setInterval(cycle3eEdit->text().toUInt());
            }
        }
    }
    /* 重置标志位 */
    iskeepCurrentTask = false;
    getServiceItemData(index, data);
    serviceRequestHandler(data);

    if (!highPerformanceEnable) {
        QString hintMsg = QString::asprintf("<font color = #1E90FF>执行诊断服务[0x%02x],%s</font>",\
                                            data.SID, data.desc.toStdString().c_str());
        promptForAction(hintMsg);
    }

    serviceActiveTask.upTotalByte += data.requestMsg.size();
    transferAverageSpeed(serviceActiveTask.upPreTime, serviceActiveTask.upTotalByte, serviceActiveTask.upAverageByte);
    if (serviceActiveTask.upAverageByte / 1024 > 10) {
        upTransferSpeedLable->setText(QString("%1 KB/s").arg(serviceActiveTask.upAverageByte / 1024));
    } else {
        upTransferSpeedLable->setText(QString("%1 byte/s").arg(serviceActiveTask.upAverageByte));
    }

    // realTimeLog(QString("通道:%1, UDS请求:%2").arg(channelName).arg(QString(data.desc)));
    if (doipConnMap.contains(channelName)) {
        DoipClientConnect *conn = doipConnMap.find(channelName).value();
        if (conn) {
            if (data.tatype.contains("function")) {
                if (doipResponseFuncAddr->text().size()) {
                    data.ta = doipResponseFuncAddr->text().toUInt(0, 16);
                }
            } else if (data.tatype.contains("physical")) {
                if (doipResponsePhysAddr->text().size()) {
                    data.ta = doipResponsePhysAddr->text().toUInt(0, 16);
                }
            }
            if (conn->diagnosisRequest(data.requestMsg.constData(), \
                    data.requestMsg.size(), data.ta, data.isspuress, data.timeout) > 0) {
                /* 发送成功 */
                sendSuccess = true;
            }
        }
    }
    else if (channelName == adbDiagChannelName) {
        sendSuccess = adbDiagRequest(data.requestMsg.constData(), \
                                     data.requestMsg.size(), data.ta, data.isspuress, data.timeout);
    }

    else {
        if (data.tatype.contains("function")) {
            if (canResponseFuncAddr->text().size()) {
                data.ta = canResponseFuncAddr->text().toUInt(0, 16);
            }
        } else if (data.tatype.contains("physical")) {
            if (canResponsePhysAddr->text().size()) {
                data.ta = canResponsePhysAddr->text().toUInt(0, 16);
            }
        }
        // CANDiagPcap.saveCANCapturePackets((const unsigned char *)data.requestMsg.constData(), data.requestMsg.size());
        sendSuccess = can->sendRequest(data.requestMsg.constData(), data.requestMsg.size(), \
                                       canRequestAddr->text().toUInt(nullptr, 16), data.ta, data.isspuress, data.timeout);
    }
    if (sendSuccess) {
#if 1
        QList<QStandardItem *> itemList;
        for (int nn = 0; nn < serviceSetModel->columnCount(); nn++) {
            QStandardItem *item = serviceSetModel->item(index, nn);
            itemList.append(item);
        }
        setActiveServiceFlicker(itemList);
#endif
    } else {
        setTableItemColor(serviceSetModel, index, serviceItemStateColor[serviceItemTestWarn]);
    }

    return true;
}

void UDS::serviceListReset(QStandardItemModel *model)
{
#ifdef __HAVE_CHARTS__
    chartView->hide();
#endif /* __HAVE_CHARTS__ */
    serviceItemStatis[serviceItemNotTest] = serviceItemStatis[serviceItemAll];
    serviceItemStatisLabel[serviceItemNotTest]->setText(QString("未测试:%1").arg(serviceItemStatis[serviceItemNotTest]));
    serviceItemStatis[serviceItemTestPass] = 0;
    serviceItemStatisLabel[serviceItemTestPass]->setText(QString("通过:%1").arg(serviceItemStatis[serviceItemTestPass]));
    serviceItemStatis[serviceItemTestFail] = 0;
    serviceItemStatisLabel[serviceItemTestFail]->setText(QString("失败:%1").arg(serviceItemStatis[serviceItemTestFail]));
    serviceItemStatis[serviceItemTestWarn] = 0;
    serviceItemStatisLabel[serviceItemTestWarn]->setText(QString("警告:%1").arg(serviceItemStatis[serviceItemTestWarn]));
    for (int index = 0; index < model->rowCount(); index++) {
        for (int nn = 0; nn < model->columnCount(); nn++) {
            QStandardItem *item = model->item(index, nn);
            if (item) {

            }
        }
        if (index % 2) {
            setTableItemColor(model, index, "#F5F5F5");
        } else {
            setTableItemColor(model, index, "#FFFFFF");
        }
    }
    if (model == serviceSetModel) {
        serviceSetView->verticalHeader()->setMinimumSectionSize(30);
        for (int row = 0; row < model->rowCount(); row++) {
            serviceSetView->verticalHeader()->resizeSection(row, 30);
            if (serviceSetModel->item(row, udsViewResponseColumn)) {
                serviceSetModel->item(row, udsViewResponseColumn)->setText(" ");
            }
            if (serviceSetModel->item(row, udsViewElapsedTimeColumn)) {
                serviceSetModel->item(row, udsViewElapsedTimeColumn)->setText(" ");
            }
            if (serviceSetModel->item(row, udsViewExpectResponseColumn)) {
                serviceSetModel->item(row, udsViewExpectResponseColumn)->setText(" ");
            }
            if (serviceSetModel->item(row, udsViewResultColumn)) {
                serviceSetModel->item(row, udsViewResultColumn)->setText(serviceItemStateColor[serviceItemNotTest]);
            }
        }
    }
}

void UDS::visualizationTestResult()
{
#ifdef __HAVE_CHARTS__
    if (serviceSetActive()) {
        return ;
    }

    if (serviceTestResultCheckBox->checkState() != Qt::Checked) {
        return ;
    }

    axisY->append("");
    QBarSet *notTest = new QBarSet("未测试");
    notTest->setColor(serviceItemStateColor[serviceItemNotTest]);
    QBarSet *passTest = new QBarSet("通过");
    passTest->setColor(serviceItemStateColor[serviceItemTestPass]);
    QBarSet *failTest = new QBarSet("失败");
    failTest->setColor(serviceItemStateColor[serviceItemTestFail]);
    QBarSet *warnTest = new QBarSet("警告");
    warnTest->setColor(serviceItemStateColor[serviceItemTestWarn]);
    *notTest << serviceItemStatis[serviceItemNotTest];
    *passTest << serviceItemStatis[serviceItemTestPass];
    *failTest << serviceItemStatis[serviceItemTestFail];
    *warnTest << serviceItemStatis[serviceItemTestWarn];

    chart->removeAllSeries();
    QHorizontalPercentBarSeries *series = new QHorizontalPercentBarSeries();
    series->append(notTest);
    series->append(passTest);
    series->append(failTest);
    series->append(warnTest);
    chart->addSeries(series);

    axisY->clear();
    series->attachAxis(axisY);
    series->attachAxis(axisX);
    chartView->setChart(chart);
    chartView->show();
#endif /* __HAVE_CHARTS__ */
}

bool UDS::abortServiceTask()
{
    // CANDiagPcap.stopCANCapturePackets();
    if (IPDiagPcapCheckBox->checkState() == Qt::Checked &&\
        doipDiagCheckBox->checkState() == Qt::Checked) {
        diagPcap.stopIPCapturePackets();
    }
    serviceActiveTask.responseTimer.stop();
    addDidDescBtn->setEnabled(true);
    addDtcDescBtn->setEnabled(true);
    addServiceBtn->setEnabled(true);
    transfer36MaxlenEdit->setEnabled(true);
    serviceSetResetBtn->setEnabled(true);
    loadTestProjectBtn->setEnabled(true);
    saveTestProjectBtn->setEnabled(true);
    delTestProjectBtn->setEnabled(true);
    serviceSetclearBtn->setEnabled(true);
    testResultCheckBox->setEnabled(true);
    cycleNumberEdit->setEnabled(true);
    TestProjectNameEdit->setEnabled(true);
    adbDiagCheckBox->setEnabled(true);
    canDiagCheckBox->setEnabled(true);
    doipDiagCheckBox->setEnabled(true);
    progressWidget->setHidden(true);
    sendListBtn->setEnabled(true);
    sendAbortListBtn->setEnabled(false);
    serviceActiveTask.indexRow = -1;
    activeServiceFlicker.isvalid = false;
    if (transferFile.isOpen()) {
        transferFile.close();
    }
    visualizationTestResult();

    return true;
}



bool UDS::readServiceSet(QString filename)
{
    QFileInfo fileInfo(filename);
    QFile file(filename);
    if (!fileInfo.isFile()) {
        return false;
    }
    serviceSetModel->removeRows(0, serviceSetModel->rowCount());
    file.open(QIODevice::ReadOnly);
    QByteArray linedata;
    do {
        linedata.clear();
        linedata = file.readLine();
        if (linedata.size()) {
            QString config = QString(linedata).toUtf8().data();
            serviceItem data = serviceItemDataDecode(config);
            addServiceItem(data);
        }
    } while (linedata.size() > 0);
    file.close();
    udsViewSetColumnHidden(false);

    return true;
}

bool UDS::serviceSetActive()
{
    return projectListIndex >= 0;
}

void UDS::promptForAction(QString text)
{
    promptActionLabel->setText(text);
}

bool UDS::serviceResponseHandle(QByteArray response, serviceItem &data)
{
    udsParse(response);
    if (UDSConditionMap.contains(data.UDSFinishCondition)) {
        if (data.UDSFinishMaxNumber == 0 || serviceActiveTask.runtotal < data.UDSFinishMaxNumber) {
            UDSFinishCondition cond = UDSConditionMap[data.UDSFinishCondition];
            QString UDSFinishData = data.UDSFinishData;
            QByteArray udsdata = QByteArray::fromHex(UDSFinishData.remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
            if (cond == UDSFinishEqualTo) {
                if (udsdata == response) {
                    iskeepCurrentTask = false;
                } else {
                    iskeepCurrentTask = true;
                }
            }
            else if (cond == UDSFinishUnEqualTo) {
                if (udsdata != response) {
                    iskeepCurrentTask = false;
                } else {
                    iskeepCurrentTask = true;
                }
            }
            else if (cond == UDSFinishRegexMatch) {
                QRegExp regexp(data.UDSFinishData);
                QRegExpValidator v(regexp, 0);
                int pos = 0;
                QString inputStr = response.toHex(' ');

                if (!regexp.isValid()) {
                    realTimeLog(QString::asprintf("[%s][%d]正则表达式不合法:%s", __FUNCTION__, __LINE__, \
                                                  data.UDSFinishData.toStdString().c_str()));
                    iskeepCurrentTask = false;
                }
                else {
                    if (v.validate(inputStr, pos) == QValidator::Acceptable) {
                        /* 匹配到了指定应答字符串，结束应答等待 */
                        realTimeLog(QString::asprintf("[%s][%d]匹配到了指定应答字符串，结束应答等待[%s][%s]",\
                                                      __FUNCTION__, __LINE__, inputStr.toStdString().c_str(), \
                                                      data.UDSFinishData.toStdString().c_str()));
                        iskeepCurrentTask = false;
                    } else {
                        iskeepCurrentTask = true;
                    }
                }
            }
            else if (cond == UDSFinishRegexMismatch) {
                QRegExp regexp(data.UDSFinishData);
                QRegExpValidator v(regexp, 0);
                int pos = 0;
                QString inputStr = response.toHex(' ');

                if (!regexp.isValid()) {
                    realTimeLog(QString::asprintf("[%s][%d]正则表达式不合法:%s", __FUNCTION__, __LINE__, \
                                                  data.UDSFinishData.toStdString().c_str()));
                    iskeepCurrentTask = false;
                }
                else {
                    if (v.validate(inputStr, pos) != QValidator::Acceptable) {
                        /* 没有匹配到指定应答字符串，结束应答等待 */
                        realTimeLog(QString::asprintf("[%s][%d]没有匹配到指定应答字符串，结束应答等待[%s][%s]",\
                                                      __FUNCTION__, __LINE__, inputStr.toStdString().c_str(), \
                                                      data.UDSFinishData.toStdString().c_str()));
                        iskeepCurrentTask = false;
                    } else {
                        iskeepCurrentTask = true;
                    }
                }
            }
        }
    }

    if (!(response.size() >= 2)) {
        return true;
    }
    quint8 sid = (quint8)response.at(0);

    if (sid == (0x40 | SecurityAccess) && (quint8)response.at(1) % 2) {
        securityAccessSeed.clear();
        securityAccessKey.clear();
        securityAccessSeed.append(response.constData() + 2, response.size() - 2);
        qDebug() << "securityAccessSeed:" << securityAccessSeed;
        QFileInfo fileInfo(securityDllDir + data.securityAccess + ".dll");
        if (QFileInfo(fileInfo).exists()) {
            qDebug() << fileInfo.filePath() << " exists";
            QLibrary lib(fileInfo.filePath());
            if(!lib.load()) {
                qDebug() << __FUNCTION__ << "security_access_generate_key" << "load err";
                return false;
            }
            unsigned char key[4096] = {0};
            typedef unsigned int (*security_access_generate_key)(unsigned char *, unsigned int , unsigned char *, unsigned int );
            auto func = (security_access_generate_key)lib.resolve("security_access_generate_key");
            if (func) {
                auto val = func((unsigned char*)securityAccessSeed.constData(), securityAccessSeed.size(), key, sizeof(key));
                securityAccessKey.append((const char *)key, val);
                qDebug() << "security_access_generate_key securityAccessKey:" << securityAccessKey;
                qDebug() << __FUNCTION__ << "security_access_generate_key" << val;
                lib.unload();
                return true;
            }

            /* 兼容周立功的算法文件 */
            typedef int (*ZLGKey)(
                const unsigned char*    iSeedArray,        // seed数组
                unsigned short          iSeedArraySize,    // seed数组大小
                unsigned int            iSecurityLevel,    // 安全级别 1,3,5...
                const char*             iVariant,          // 其他数据, 可设置为空
                unsigned char*          iKeyArray,         // key数组, 空间由用户构造
                unsigned short*         iKeyArraySize      // key数组大小, 函数返回时为key数组实际大小
            );
            auto func1 = (ZLGKey)lib.resolve("ZLGKey");
            if (func1) {
                unsigned short keyarrsize = sizeof(key);
                auto val = func1((const unsigned char*)securityAccessSeed.constData(), \
                                 (unsigned short)securityAccessSeed.size(), \
                                 (unsigned int)response.at(1) + 1, 0, \
                                 key, &keyarrsize);
                if (val == 0 && keyarrsize > 0) {
                    securityAccessKey.append((const char *)key, keyarrsize);
                }
                qDebug() << "ZLGKey securityAccessKey:" << securityAccessKey;
                qDebug() << __FUNCTION__ << "ZLGKey keyarrsize" << keyarrsize;
                lib.unload();
                return true;
            }
            /* 兼容CANoe的算法文件 */
            typedef int (*GenerateKeyEx)(
              const unsigned char* ipSeedArray,          /* Array for the seed [in] */
              unsigned int iSeedArraySize,               /* Length of the array for the seed [in] */
              const unsigned int iSecurityLevel,         /* Security level [in] */
              const char* ipVariant,                     /* Name of the active variant [in] */
              unsigned char* iopKeyArray,                /* Array for the key [in, out] */
              unsigned int iMaxKeyArraySize,             /* Maximum length of the array for the key [in] */
              unsigned int& oActualKeyArraySize);         /* Length of the key [out] */
            auto func2 = (GenerateKeyEx)lib.resolve("GenerateKeyEx");
            if (func2) {
                unsigned int oActualKeyArraySize = 0;
                auto val = func2((const unsigned char*)securityAccessSeed.constData(), \
                                 (unsigned int)securityAccessSeed.size(), \
                                 (unsigned int)response.at(1) + 1, \
                                 0, (unsigned char*)key,\
                                 (unsigned int)sizeof(key), \
                                 oActualKeyArraySize);
                if (val == 0 && oActualKeyArraySize > 0) {
                    securityAccessKey.append((const char *)key, oActualKeyArraySize);
                }
                qDebug() << "GenerateKeyEx securityAccessKey:" << securityAccessKey;
                qDebug() << __FUNCTION__ << "GenerateKeyEx oActualKeyArraySize" << oActualKeyArraySize;
                lib.unload();
                return true;
            }

            lib.unload();
        }
    }
    else if (sid == (0x40 | RequestFileTransfer)) {
        modeOfOperation = response.at(1);
        if (modeOfOperation == AddFile || modeOfOperation == ReplaceFile) {
            if (!(response.size() >= 3)) {
                return false;
            }
            lengthFormatIdentifier = response.at(2);
            for (int nbyte = 0; nbyte < lengthFormatIdentifier; nbyte++) {
                uint8_t byte = 0;

                byte = response.at(3 + nbyte);
                maxNumberOfBlockLength |=  byte << ((lengthFormatIdentifier - nbyte - 1) * 8);
            }
            dataFormatIdentifier = response.at(3 + lengthFormatIdentifier);
        }
        if (transfer36Maxlen > 0) {
            /* 如果36服务有配置最大传输长度就使用配置的，否则使用38服务反馈的 */
            maxNumberOfBlockLength = transfer36Maxlen;
            qDebug() << "Reset maxNumberOfBlockLength:" << maxNumberOfBlockLength;
        }

        qDebug() << "modeOfOperation:" << modeOfOperation;
        qDebug() << "lengthFormatIdentifier:" << lengthFormatIdentifier;
        qDebug() << "maxNumberOfBlockLength:" << maxNumberOfBlockLength;
        qDebug() << "dataFormatIdentifier:" << dataFormatIdentifier;
        fileTransferTotalSize = 0; /* 统计传输字节数和 */
        fileTransferCount = 1; /* 文件传输序列号从1开始 */
        qDebug() << "transferFilePath:" << localFilePath;
        if (transferFile.isOpen()) {
            transferFile.close();
        }
        QFileInfo fileInfo(localFilePath);
        if (fileInfo.isAbsolute()) {
            /* 如果文件是绝对路径就在这个路径下打开 */
            if (fileInfo.isFile()) {
                transferFile.setFileName(localFilePath);
                transferFile.open(QIODevice::ReadOnly);
            }
            else {
                realTimeLog(QString::asprintf("[%s][%d]文件不存在:%s", __FUNCTION__, __LINE__, localFilePath.toStdString().c_str()));
                QMessageBox::warning(nullptr, tr("38服务文件确认"), tr(QString("文件不存在:%1").arg(localFilePath).toStdString().c_str()));
                return false;
            }
        }
        else {
            /* 文件如果是相对路径就在当前指定目录下寻找 */
            fileInfo.setFile(servicetransferFileDir + localFilePath);
            if (fileInfo.isFile()) {
                transferFile.setFileName(servicetransferFileDir + localFilePath);
                transferFile.open(QIODevice::ReadOnly);
            }
            else {
                realTimeLog(QString::asprintf("[%s][%d]文件不存在:%s", __FUNCTION__, __LINE__, localFilePath.toStdString().c_str()));
                QMessageBox::warning(nullptr, tr("38服务文件确认"), tr(QString("文件不存在:%1").arg(servicetransferFileDir + localFilePath).toStdString().c_str()));
                return false;
            }
        }
        /* 设置36服务传输进度条 */
        if (!highPerformanceEnable) {
            promptActionProgress->setRange(0, transferFile.size());
            promptActionProgress->setValue(0);
        }
    }
    else if (sid == (0x40 | TransferData)) {
        fileTransferCount++; /* 文件传输序列号递增 */
        // qDebug() << "fileTransferTotalSize:" << fileTransferTotalSize;
        // qDebug() << "transferFile.size():" << transferFile.size();
        fileTransferTotalSize += (maxNumberOfBlockLength - 2);
        /* 显示36传输进度 */
        if (!highPerformanceEnable) {
            promptActionProgress->show();
            promptActionProgress->setValue(fileTransferTotalSize);
        }
        /* 判断文件是否传输完成 */
        if (fileTransferTotalSize >= transferFile.size() || data.requestMsg.size() > 1) {
            /* 传输完成, 或者36服务本身就配置有数据，优先使用36服务配置的数据，不使用打开的文件 */
            promptActionProgress->hide();
            iskeepCurrentTask = false;
            transferFile.close();
        } else {
            /* 传输未完成 */
            iskeepCurrentTask = true;
        }
    }
    else if (sid == (0x40 | DiagnosticSessionControl)) {
        if (response.size() >= 6) {
            this->P2ServerMax = response.at(2) << 8 | response.at(3);
            this->P2EServerMax = response.at(4) << 8 | response.at(5);
            realTimeLog(QString::asprintf("p2 server max => %d ms", this->P2ServerMax));
            realTimeLog(QString::asprintf("p2 e server max => %d ms", this->P2EServerMax));
        }
    }

    return true;
}

bool UDS::keepCurrentTask()
{
    return iskeepCurrentTask;
}

void UDS::udsViewSetColumnHidden(bool hidden)
{
    if (hidden == false) {
        /* 如果要显示该行，首先要判断是否允许显示 */
        if (ConfigBtnCheckBox->checkState() == Qt::Checked) {
            serviceSetView->setColumnHidden(udsViewConfigBtnColumn, hidden);
        }
        if (SendBtnCheckBox->checkState() == Qt::Checked) {
            serviceSetView->setColumnHidden(udsViewSendBtnColumn, hidden);
        }
        if (DelBtnCheckBox->checkState() == Qt::Checked) {
            serviceSetView->setColumnHidden(udsViewDelBtnColumn, hidden);
        }
        if (UpBtnCheckBox->checkState() == Qt::Checked) {
            serviceSetView->setColumnHidden(udsViewUpBtnColumn, hidden);
        }
        if (DownBtnCheckBox->checkState() == Qt::Checked) {
            serviceSetView->setColumnHidden(udsViewDownBtnColumn, hidden);
        }
    }
    else {
        serviceSetView->setColumnHidden(udsViewConfigBtnColumn, hidden);
        serviceSetView->setColumnHidden(udsViewSendBtnColumn, hidden);
        serviceSetView->setColumnHidden(udsViewDelBtnColumn, hidden);
        serviceSetView->setColumnHidden(udsViewUpBtnColumn, hidden);
        serviceSetView->setColumnHidden(udsViewDownBtnColumn, hidden);
    }
}

bool UDS::createAdbProcess()
{
    if(adbListenProcess == nullptr) {
        adbListenProcess = new QProcess(this);
        adbListenExecArgs = QStringList() << "shell" << "chmod" << "+x" << "/tmp/runtime_debug_api";
        adbListenTimer = new QTimer(this);
        adbListenTimer->setInterval(1000);
        // adbListenTimer->start();
        connect(adbListenTimer, &QTimer::timeout, this, [this](){
            qDebug() << adbListenProcess->state() << adbListenExecArgs;
            if (adbListenProcess->state() == QProcess::Running) {
                adbListenProcess->kill();
            } else {
                adbListenProcess->start("./bin/adb/adb.exe", adbListenExecArgs);
            }
            qDebug() << adbListenProcess->state() << adbListenExecArgs;
        });
        connect(adbListenProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
              [=](int exitCode, QProcess::ExitStatus exitStatus){
            qDebug() << "exitCode:" << exitCode;
            qDebug() << "exitStatus:" << exitStatus;
            qDebug() << "adbprocess finished:" << adbListenProcess->readAllStandardOutput();
            if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                adbStateLabel->setText("<font color = green><u>登录成功</u></font>");
                adbListenTimer->stop();
            }
        });
        connect(adbListenProcess, &QProcess::readyReadStandardOutput,
              [=](){
            QByteArray readStr = adbListenProcess->readAllStandardOutput();
            qDebug() << "adbLoginHintEdit|" << adbLoginHintEdit->lineEdit()->text().toStdString().data();
            qDebug() << "readStr|" << readStr;
            adbStateLabel->setText("<font><u>登录中...</u></font>");
            if (readStr.contains(adbLoginHintEdit->lineEdit()->text().toStdString().data())) {
                qDebug() << adbLoginHintEdit->lineEdit()->text();
                qDebug() << QByteArray(adbLoginNameEdit->lineEdit()->text().toStdString().data()).append("\n");
                adbListenProcess->write(QByteArray(adbLoginNameEdit->lineEdit()->text().toStdString().data()).append("\n"));
            } else if (readStr.contains(adbPasswordHintEdit->lineEdit()->text().toStdString().data())) {
                qDebug() << adbPasswordEdit->lineEdit()->text();
                qDebug() << QByteArray(adbPasswordEdit->lineEdit()->text().toStdString().data()).append("\n");
                adbListenProcess->write(QByteArray(adbPasswordEdit->lineEdit()->text().toStdString().data()).append("\n"));
            } else if (readStr.contains("No such file or directory")) {
                adbListenExecArgs = QStringList() << "push" << "./bin/runtime_debug/dp/runtime_debug_api" << "/tmp/";
            } else {
                adbListenExecArgs = QStringList() << "shell" << "chmod" << "+x" << "/tmp/runtime_debug_api";
            }
        });
    }
    if (adbCmdExecProcess == nullptr) {
        adbCmdTimeoutTimer = new QTimer();
        connect(adbCmdTimeoutTimer, &QTimer::timeout, this, [this](){
            QByteArray response;
            if (adbActiveTask.isvalid && adbActiveTask.issuppress) {
                diagnosisResponseHandle(adbDiagChannelName, DoipClientConnect::normalFinish, response);
            } else {
                diagnosisResponseHandle(adbDiagChannelName, DoipClientConnect::TimeoutFinish, response);
            }
            adbActiveTask.isvalid = false;
            adbActiveTask.issuppress = false;
            adbCmdTimeoutTimer->stop();
            adbListenTimer->start();
            adbCmdExecProcess->kill();
        });
        adbCmdExecProcess = new QProcess(this);
        connect(adbCmdExecProcess, &QProcess::readyReadStandardOutput,
              [=](){
                QByteArray readStr = adbCmdExecProcess->readAllStandardOutput();

                QString responseStr = QString(readStr).remove("Runtime dbg api recv: ");//.remove("\r\r\n");
                qDebug() << "response:" << responseStr;
                QStringList strList = responseStr.split("\r\r\n", QString::SkipEmptyParts);
                for (int i = 0; i < strList.size(); i++) {
                    QJsonParseError err;
                    QJsonObject responseObj = QJsonDocument::fromJson(strList.at(i).toUtf8(), &err).object();
                    if (err.error)
                        return ;
                    if (responseObj.contains("command") && \
                        responseObj["command"] == "CallUdsApi" && \
                        responseObj.contains("data")) {
                        QByteArray response = QByteArray::fromHex(responseObj["data"].toString().remove(QRegExp("[^0-9a-fA-F]")).toUtf8());
                        diagnosisResponseHandle("UDSonADB", DoipClientConnect::normalFinish, response);
                        adbActiveTask.isvalid = false;
                        adbActiveTask.issuppress = false;
                        adbCmdTimeoutTimer->stop();
                        adbListenTimer->stop();
                        adbCmdExecProcess->kill();
                    }
                }
        });

        connect(adbCmdExecProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
              [=](int exitCode, QProcess::ExitStatus exitStatus){
            qDebug() << "exitCode:" << exitCode;
            qDebug() << "exitStatus:" << exitStatus;
            qDebug() << "adbprocess finished:" << adbCmdExecProcess->readAllStandardOutput();
            if (adbActiveTask.isvalid) {
                QByteArray response;
                if (adbActiveTask.issuppress) {
                    diagnosisResponseHandle(adbDiagChannelName, DoipClientConnect::normalFinish, response);
                } else {
                    diagnosisResponseHandle(adbDiagChannelName, DoipClientConnect::nackFinish, response);
                }
                adbActiveTask.isvalid = false;
                adbCmdTimeoutTimer->stop();
            }

            if (adbWaitTask.isvalid) {
                adbWaitTask.isvalid = false;
                adbCmdTimeoutTimer->setInterval(adbWaitTask.adbCmdTimeoutInterval);
                adbCmdTimeoutTimer->start();
                adbCmdExecProcess->start("./bin/adb/adb.exe", adbWaitTask.adbWaitExecArgs);

                adbActiveTask.isvalid = true;
                adbActiveTask.issuppress = adbWaitTask.issuppress;
                adbActiveTask.adbWaitExecArgs = adbWaitTask.adbWaitExecArgs;
                adbActiveTask.adbCmdTimeoutInterval = adbWaitTask.adbCmdTimeoutInterval;
            }
        });
    }

  //
    return true;
}
bool UDS::adbDiagRequest(const char *buf, quint32 len, quint16 ta, bool supression, quint16 timeout)
{
    if (adbWaitTask.isvalid) {
        return false;
    }
    QByteArray data;

    data.append(buf, len);
    QString udsmsg = QString::asprintf("{\\\"command\\\":\\\"CallUdsApi\\\",\\\"SA\\\":\\\"%02x\\\",\\\"TA\\\":\\\"%02x\\\",\\\"data\\\":\\\"%s\\\"}", 0, ta, data.toHex().toStdString().data());
    QStringList args = QStringList() << "shell" << "/tmp/./runtime_debug_api" << "-d" << udsmsg << "-t" << QString("%1").arg(timeout);
    if (adbCmdExecProcess->state() == QProcess::Running) {
        adbWaitTask.isvalid = true;
        adbWaitTask.issuppress = supression;
        adbWaitTask.adbWaitExecArgs = args;
        adbWaitTask.adbCmdTimeoutInterval = timeout;
        qDebug() << "2222222222222222222";
        return false;
    }
    adbActiveTask.isvalid = true;
    adbActiveTask.issuppress = supression;
    adbActiveTask.adbWaitExecArgs = args;
    adbActiveTask.adbCmdTimeoutInterval = timeout;
    adbCmdTimeoutTimer->setInterval(adbActiveTask.adbCmdTimeoutInterval);
    adbCmdTimeoutTimer->start();
    adbCmdExecProcess->start("./bin/adb/adb.exe", adbActiveTask.adbWaitExecArgs);

    return true;
}

#define DTC_POWERTRAIN (0b00)
#define DTC_CHASSIS    (0b01)
#define DTC_BODY       (0b10)
#define DTC_NETWORK    (0b11)
QString dtc_SAE_J2012_DA(quint8 dtc[4])
{
    QString str;

    if (dtc[0] >> 6 == DTC_POWERTRAIN) {
        str.append('P');
    }
    else if (dtc[0] >> 6 == DTC_CHASSIS) {
        str.append('C');
    }
    else if (dtc[0] >> 6 == DTC_BODY) {
        str.append('B');
    }
    else if (dtc[0] >> 6 == DTC_NETWORK) {
        str.append('U');
    }
    str.append(QString::asprintf("%X%X%02X%02X", (dtc[0] >> 4) & 0x03, dtc[0] & 0x0f, dtc[1], dtc[2]));

    return str;
}

QString UDS::udsParse(QByteArray msg)
{
    stream_t Sp;
    quint8 sid = 0;
    quint8 sub = 0;
    quint16 did = 0;
    QString formatStr;

    // formatStr.append("\n");
    stream_init(&Sp, (unsigned char*)msg.data(), msg.size());
    sid = stream_byte_read(&Sp);
    if (sid == 0x7f) {
        formatStr.append("Response: negative response \n");
        quint8 rsid = stream_byte_read(&Sp);
        quint8 nrc = stream_byte_read(&Sp);
        formatStr.append(QString("Service: %1 \n").arg(serviceDescMap[rsid]));
        formatStr.append(QString("Negative code: %1 \n").arg(positiveResponseCodeDesc[nrc]));
    } else if (sid & 0x40) {
        sid &= (~0x40);
        if (serviceDescMap.contains(sid)) {
            formatStr.append("Response: positive response \n");
            // qDebug() << "Service:" << serviceDescMap[sid];
            formatStr.append(QString("Service: %1 \n").arg(serviceDescMap[sid]));
        }
        if (serviceSubDescMap.contains(sid)) {
            sub = stream_byte_read(&Sp);
            if (serviceSubDescMap[sid].contains(sub)) {
                formatStr.append(QString("Sub-function: %1 \n").arg(serviceSubDescMap[sid][sub]));
            }
        }

        if (sid == DiagnosticSessionControl) {
            quint16 P2Server_max = stream_2byte_read(&Sp);
            formatStr.append(QString::asprintf("P2Server_max: %d \n", P2Server_max));
            P2Server_max = stream_2byte_read(&Sp);
            formatStr.append(QString::asprintf("P2*Server_max: %d \n", P2Server_max));
        }
        else if (sid == SecurityAccess) {
            unsigned char dataRecord[64] = {0};
            int datalen = 0;

            datalen = stream_left_len(&Sp);
            stream_nbyte_read(&Sp, dataRecord, datalen);
            if (sub % 2) {
                formatStr.append(QString::asprintf("Seed: %s \n", QByteArray((const char *)dataRecord, datalen).toHex(' ').data()));
            } else {
                formatStr.append(QString::asprintf("Key: %s \n", QByteArray((const char *)dataRecord, datalen).toHex(' ').data()));
            }
        }
        else if ((sid == ReadDataByIdentifier || \
                  sid == RoutineControl || \
                  sid == WriteDataByIdentifier) && \
                 (stream_left_len(&Sp) >= 2)) {
            unsigned char dataRecord[4096] = {0};
            unsigned int datalen = 0;
            did = stream_2byte_read(&Sp);
            datalen = stream_left_len(&Sp);
            stream_nbyte_read(&Sp, dataRecord, datalen);
            formatStr.append(QString::asprintf("Data identifier: %04x ", did));
            if (didDescDictionary.contains(QString::asprintf("%04x", did))) {
                formatStr.append(didDescDictionary[QString::asprintf("%04x", did)] + "\n");
            } else {
                formatStr.append("\n");
            }
            if (QString((const char *)dataRecord).size() == (int)datalen) {
                formatStr.append(QString::asprintf("Data record(string): %s \n", dataRecord));
            }
            QByteArray dataByte;
            dataByte.append((const char *)dataRecord, datalen);
            formatStr.append(QString::asprintf("Data record(hex): %s \n", dataByte.toHex(' ').data()));
        }
        else if (sid == ReadDTCInformation) {
            if (sub == reportDTCByStatusMask || \
                sub == reportSupportedDTCs || \
                sub == reportFirstTestFailedDTC || \
                sub == reportFirstConfirmedDTC || \
                sub == reportMostRecentTestFailedDTC || \
                sub == reportMostRecentConfirmedDTC || \
                sub == reportMirrorMemoryDTCByStatusMask || \
                sub == reportEmissionsOBDDTCByStatusMask || \
                sub == reportDTCWithPermanentStatus) {
                unsigned char dtc[4] = {0};

                quint8 mask = stream_byte_read(&Sp);
                formatStr.append(QString::asprintf("DTC Status mask : %02x \n",mask));
                int dtcnum = stream_left_len(&Sp) / 4;
                for (int nn = 0; nn < dtcnum; nn++) {
                    stream_nbyte_read(&Sp, dtc, sizeof(dtc));
                    QString dtcstr = dtc_SAE_J2012_DA(dtc);
                    if (dtcDescDictionary.contains(dtcstr)) {
                        formatStr.append(QString::asprintf("DTC<%d> describe : ", nn + 1) + dtcDescDictionary[dtcstr] + "\n");
                    }
                    formatStr.append(QString::asprintf("DTC<%d> : ", nn + 1));
                    dtcstr.append(QString::asprintf("[%02X%02X%02X%02X]", dtc[0], dtc[1], dtc[2], dtc[3]));
                    if ((dtc[3] & 0x09) == 0x09) {
                        dtcstr.append("(实时故障)");
                    } else if ((dtc[3] & 0x08) == 0x08) {
                        dtcstr.append("(历史故障)");
                    }
                    formatStr.append(dtcstr + "\n");
                }
            }
        }

    } else {
        if (serviceDescMap.contains(sid)) {
            // qDebug() << "Service:" << serviceDescMap[sid];
            formatStr.append(QString("Service: %1 \n").arg(serviceDescMap[sid]));
        }
        if (serviceSubDescMap.contains(sid)) {
            sub = stream_byte_read(&Sp);
            if (serviceSubDescMap[sid].contains(sub)) {
                formatStr.append(QString("Sub-function: %1 \n").arg(serviceSubDescMap[sid][sub]));
            }
        }
        if (sid == 0x22 || sid == 0x31) {
            did = stream_2byte_read(&Sp);
            formatStr.append(QString::asprintf("Data identifier: %04x \n", did));
        }
    }
    formatStr.append("\n");
    //QJsonDocument doc(parseObj);
    return formatStr;
}

bool UDS::addDtcDictionaryItem(QString text)
{
    QJsonParseError err;

    QJsonObject dtcObj = QJsonDocument::fromJson(text.toUtf8(), &err).object();
    if (err.error)
        return false;

    if (dtcObj.contains("DTC") && dtcObj.contains("Describe")) {
        dtcDescDictionary.insert(dtcObj["DTC"].toString(), dtcObj["Describe"].toString());
    }

    return true;
}

bool UDS::addDidDictionaryItem(QString text)
{
    QJsonParseError err;

    QJsonObject dtcObj = QJsonDocument::fromJson(text.toUtf8(), &err).object();
    if (err.error)
        return false;

    if (dtcObj.contains("DID") && dtcObj.contains("Describe")) {
        didDescDictionary.insert(dtcObj["DID"].toString(), dtcObj["Describe"].toString());
    }

    return true;
}

QString UDS::generateTestResultFileName()
{
    QString projectName;

    resultFileName.clear();
    if (projectListTimer->isActive()) {
        projectName = projectNameListComBox->lineEdit()->text();
    } else {
        projectName = TestProjectNameEdit->lineEdit()->text();
    }
    resultFileName = resultFileName + projectName.split(".").at(0);
    if (cycleNumberEdit->text().toUInt() > 0) {
        resultFileName = resultFileName + "循环测试" + cycleNumberEdit->text() + "次_";
    }
    resultFileName = resultFileName + QDateTime::currentDateTime().toString( + "yyyy_MM_dd_hh_mm_ss");
    resultFileName = serviceTestResultDir + resultFileName + ".xlsx";
    qDebug() << resultFileName;

    return resultFileName;
}

bool UDS::serviceResponseToTable(int indexRow, QByteArray response)
{
    int columnHeight = 30;
    serviceItem data;

    getServiceItemData(indexRow, data);

    serviceSetView->verticalHeader()->setMinimumSectionSize(30);
    QStandardItem *item = serviceSetModel->item(indexRow, udsViewResponseColumn);
    if (item) {
        if (UDSResponseParseCheckBox->checkState() == Qt::Checked) {
            QString parseStr = udsParse(response);
            item->setText(parseStr);
            item->setToolTip(QString(response.toHex(' ')));
            columnHeight = fontHeight *  parseStr.count('\n') > columnHeight ? fontHeight *  parseStr.count('\n') : columnHeight;
        } else{
            item->setText(QString(response.toHex(' ')));
            item->setToolTip(udsParse(response));
        }
    }

    item = serviceSetModel->item(indexRow, udsViewExpectResponseColumn);
    if (item) {
        if (UDSResponseParseCheckBox->checkState() == Qt::Checked) {
            //QString parseStr = udsParse(data.expectResp);
            //item->setText(parseStr);
            //item->setToolTip(QString(data.expectResp.toHex(' ')));
            //columnHeight = fontHeight *  parseStr.count('\n') > columnHeight ? fontHeight *  parseStr.count('\n') : columnHeight;
            item->setText(data.expectResponseRule);
        } else{
            item->setText(data.expectResponseRule);
            //item->setToolTip(udsParse(response));
        }
    }
    serviceSetView->verticalHeader()->resizeSection(indexRow, columnHeight);

    return true;
}

bool UDS::serviceSetDelItem(int row)
{
    if (serviceActive()) return false;

    serviceSetBackTimer->start();
    serviceSetModel->removeRow(row);
    serviceItemStatis[serviceItemAll] = serviceSetModel->rowCount();
    serviceItemStatisLabel[serviceItemAll]->setText(QString("总共:%1").arg(serviceItemStatis[serviceItemAll]));

    return true;
}

bool UDS::serviceSetChangeItem(int row)
{
    if (serviceActive()) return false;
    serviceSetBackTimer->start();
    QStandardItem *requestItem = serviceSetModel->item(row, udsViewRequestColumn);
    if (requestItem) {
        QString toolTip = requestItem->toolTip();
        stringToWidgetConfig(toolTip);
        serviceSetWidget->show();
    }
    udsRequestConfigBtn->setText(QString("修改(%1)").arg(serviceDesc->text()));
    modifyRowIndex = row;

    return true;
}

bool UDS::serviceSetMoveItem(int row, int newrow)
{
    if (serviceActive()) return false;
    if (row < 0 || row >= serviceSetModel->rowCount()) return false;
    if (newrow < 0 || newrow >= serviceSetModel->rowCount()) return false;
    serviceItem data;

    getServiceItemData(row, data);
    QList<QStandardItem *> items = serviceSetModel->takeRow(row);
    serviceSetModel->insertRow(newrow, items);
    setServiceItemOperationControl(newrow, data);
    serviceSetView->selectRow(newrow);

    return true;
}

bool UDS::projectSetMoveItem(int row, int newrow)
{
    if (serviceActive()) return false;
    if (row < 0 || row >= projectNameModel->rowCount()) return false;
    if (newrow < 0 || newrow >= projectNameModel->rowCount()) return false;

    QList<QStandardItem *> items = projectNameModel->takeRow(row);
    projectNameModel->insertRow(newrow, items);
    projectNameTable->selectRow(newrow);

    return true;
}

int UDS::transferAverageSpeed(QDateTime &preTime, qint32 &totalByte, qint32 &averageByte)
{
    QDateTime currDateTime = QDateTime::currentDateTime();
    qint64 intervalSecs = preTime.secsTo(currDateTime);
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

void UDS::serverSetFinishHandle()
{
    /* 所有任务结束 */
    if (serviceSetActive()) {
        /* 标记单条测试用例测试结果 */
        if (projectListIndex <= projectNameModel->rowCount()) {
            activeServiceProjectFlicker.isvalid = false;
            QStandardItem *item = projectNameModel->item(projectListIndex - 1, 0);
            if (serviceItemStatis[serviceItemTestFail] == 0) {
                if (item) {item->setBackground(QColor("#228B22"));}
            } else {
                if (item) {item->setBackground(QColor("#CD3333"));}
            }
        }
    }

    if (--serviceActiveTask.cyclenum >= 0) {
        /* 所有任务重复执行任务 */
        /* 重新开始所有计时 */
        if (testResultCheckBox->checkState() == Qt::Checked) {
            QString prefix;
            if (projectListTimer->isActive()) {
                /* projectListIndex 这里 -1是因为已经指向下一个了，获取当前的话需要-1 */
                QStandardItem *item = projectNameModel->item(projectListIndex - 1, 0);
                if (item) {prefix = item->text();}
            } else {
                prefix = QString("循环测试次数剩余%1").arg(serviceActiveTask.cyclenum + 1);
            }
            prefix = prefix + " " + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");
            tableToXlsx.modelToXlsx(*serviceSetModel, resultFileName, prefix, udsViewResultColumn);
        }
        allServiceTimer.restart();
        serviceActiveTask.indexRow = 0;
        getServiceItemData(serviceActiveTask.indexRow, serviceActiveTask.data);
        serviceListTaskTimer->setInterval(serviceActiveTask.data.delay);
        serviceListTaskTimer->start();
        serviceListReset(serviceSetModel);
    } else {
        /* 所有任务结束 */
        if (testResultCheckBox->checkState() == Qt::Checked) {
            QString prefix;
            if (projectListTimer->isActive()) {
                /* projectListIndex 这里 -1是因为已经指向下一个了，获取当前的话需要-1 */
                QStandardItem *item = projectNameModel->item(projectListIndex - 1, 0);
                if (item) {prefix = item->text();}
            } else {
                prefix = QString("循环测试次数剩余%1").arg(serviceActiveTask.cyclenum + 1);
            }
            prefix = prefix + " " + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss");
            tableToXlsx.modelToXlsx(*serviceSetModel, resultFileName, prefix, udsViewResultColumn);
            promptForAction(QString("<a style='color: rgb(30, 144, 255);' href=\"file:///%1\">测试结果文件：%2</a>").\
                            arg(serviceTestResultDir).arg(resultFileName));
            if (!projectListTimer->isActive()) {
                testResultCheckBox->setCheckState(Qt::Unchecked);
            }
        }
        abortServiceTask();
    }
    if (serviceActiveTask.cyclenum >= 0) {
        cycleNumberEdit->setText(QString("%1").arg(serviceActiveTask.cyclenum));
    }
}

UDS::serviceItemState UDS::serverSetItemResultState(serviceItem data, QByteArray response)
{
    enum serviceItemState itemState = serviceItemTestFail;

    QStandardItem *responseItem = serviceSetModel->item(serviceActiveTask.indexRow, udsViewResponseColumn);
    if (responseExpectMap[data.expectResponse] == noResponseExpect) {
        if (response.size() > 0) {
            itemState = serviceItemTestFail;
            responseItem->setText(QString::asprintf("预期无响应，实际响应(%s)", response.toHex(' ').toStdString().c_str()));
        }
        else itemState = serviceItemTestPass;
    }
    else if (responseExpectMap[data.expectResponse] == negativeResponseExpect ||\
             responseExpectMap[data.expectResponse] == positiveResponseExpect) {
        if (response.toHex(' ') != data.expectResponseRule) {
            itemState = serviceItemTestFail;
            responseItem->setText(QString::asprintf("预期响应%s(%s)，实际响应(%s)", \
                data.expectResponse.toStdString().c_str(), \
                data.expectResponseRule.toStdString().c_str(), response.toHex(' ').toStdString().c_str()));
        }
        else itemState = serviceItemTestPass;
    }
    else if (responseExpectMap[data.expectResponse] == responseRegexMatch) {
        QRegExp regexp(data.expectResponseRule);
        QRegExpValidator v(regexp, 0);
        int pos = 0;
        QString inputStr = response.toHex(' ');

        if (!regexp.isValid()) {
            realTimeLog(QString::asprintf("[%s][%d]正则表达式不合法:%s", __FUNCTION__, __LINE__, \
                                          data.expectResponseRule.toStdString().c_str()));
        }
        else {
            if (v.validate(inputStr, pos) == QValidator::Acceptable) {
                /* 匹配到了指定应答字符串，结束应答等待 */
                realTimeLog(QString::asprintf("[%s][%d]匹配到了指定应答字符串，应答符合预期[%s][%s]",\
                                              __FUNCTION__, __LINE__, inputStr.toStdString().c_str(), \
                                              data.expectResponseRule.toStdString().c_str()));
                itemState = serviceItemTestPass;
            } else {
                realTimeLog(QString::asprintf("[%s][%d]没有匹配到了指定应答字符串，应答不符合预期[%s][%s]",\
                                              __FUNCTION__, __LINE__, inputStr.toStdString().c_str(), \
                                              data.expectResponseRule.toStdString().c_str()));
                itemState = serviceItemTestFail;
                responseItem->setText(QString::asprintf("预期响应%s(%s)，实际响应(%s)", \
                    data.expectResponse.toStdString().c_str(), \
                    data.expectResponseRule.toStdString().c_str(), response.toHex(' ').toStdString().c_str()));
            }
        }
    }
    else if (responseExpectMap[data.expectResponse] == responseRegexMismatch) {
        QRegExp regexp(data.expectResponseRule);
        QRegExpValidator v(regexp, 0);
        int pos = 0;
        QString inputStr = response.toHex(' ');

        if (!regexp.isValid()) {
            realTimeLog(QString::asprintf("[%s][%d]正则表达式不合法:%s", __FUNCTION__, __LINE__, \
                                          data.expectResponseRule.toStdString().c_str()));
        }
        else {
            if (v.validate(inputStr, pos) == QValidator::Acceptable) {
                /* 匹配到了指定应答字符串，结束应答等待 */
                realTimeLog(QString::asprintf("[%s][%d]没有匹配到指定应答字符串，应答不符合预期[%s][%s]",\
                                              __FUNCTION__, __LINE__, inputStr.toStdString().c_str(), \
                                              data.expectResponseRule.toStdString().c_str()));
                itemState = serviceItemTestFail;
            } else {
                realTimeLog(QString::asprintf("[%s][%d]匹配到了指定应答字符串，应答符合预期[%s][%s]",\
                                              __FUNCTION__, __LINE__, inputStr.toStdString().c_str(), \
                                              data.expectResponseRule.toStdString().c_str()));
                itemState = serviceItemTestPass;
            }
        }
    }
    else {
        if (response.size() > 0 && response.at(0) == 0x7f) {
            itemState = serviceItemTestWarn;
        }
        else {
            itemState = serviceItemTestPass;
        }
    }

    return itemState;
}

bool UDS::execAdbCmd(QStringList args)
{
    Q_UNUSED(args);

    return true;
}

void UDS::setActiveServiceFlicker(QList<QStandardItem *> &item)
{
    if (item.size() <= 0) return ;

    if (item.at(0) == activeServiceFlicker.Item.at(0) && \
        activeServiceFlicker.isvalid) return ;

    activeServiceFlicker.isvalid = true;
    activeServiceFlicker.direct = true;
    activeServiceFlicker.statusLevel = 0;
    activeServiceFlicker.Item = item;
}

void UDS::setActiveServiceProjectFlicker(QList<QStandardItem *> &item)
{
    if (item.size() <= 0) return ;

    if (item.at(0) == activeServiceProjectFlicker.Item.at(0) && \
        activeServiceProjectFlicker.isvalid) return ;

    activeServiceProjectFlicker.isvalid = true;
    activeServiceProjectFlicker.direct = true;
    activeServiceProjectFlicker.statusLevel = 0;
    activeServiceProjectFlicker.Item = item;
}

void UDS::doipServerListChangeSlot(QMap<QString, DoipClientConnect *> &map)
{
    qDebug() << "doipServerListChangeSlot";
    qDebug() << map;
    doipConnMap = map;
    doipServerListBox->clear();
    QMap<QString, DoipClientConnect *>::const_iterator i = map.constBegin();
    while (i != map.constEnd()) {
        doipServerListBox->addItem(i.key());
        ++i;
    }
}

void UDS::diagnosisResponseSlot(DoipClientConnect *doipConn, DoipClientConnect::sendDiagFinishState state, QByteArray &response)
{
    diagnosisResponseHandle(doipConn->getName(), state, response);
}

void UDS::diagnosisResponseHandle(QString channelName, DoipClientConnect::sendDiagFinishState state, QByteArray &response)
{
    for (; state == DoipClientConnect::normalFinish ;) {
        serviceItem sitem;

        if (serviceActive()) {
            getServiceItemData(serviceActiveTask.indexRow, sitem);
        }
        else if (manualSendRowIndex >= 0) {
            getServiceItemData(manualSendRowIndex, sitem);
        }
        else {
            return ;
        }
        if(response.size() >= 3 && \
           (quint8)response.at(0) == 0x7f && \
            (quint8)response.at(1) == sitem.SID) {
            break; /* for */
        }
        if (response.size() >= 1 && \
           (quint8)response.at(0) == (sitem.SID | 0x40)) {
            break; /* for */
        }
        return ;
    }
    if (response.size() == 3 &&\
        (quint8)response.at(0) == (quint8)0x7f &&\
        (quint8)response.at(2) == (quint8)0x78) {
        /* 只刷新定时器 */
        realTimeLog(QString::asprintf("刷新响应超时定时器 %d ms", this->P2EServerMax * 10));
        serviceActiveTask.responseTimer.setInterval(this->P2EServerMax * 10);
        return ;
    }

    if (manualSendRowIndex >= 0) {
        serviceItem data;
        getServiceItemData(manualSendRowIndex, data);
        enum serviceItemState itemState = serviceItemTestFail;
        if (state == DoipClientConnect::normalFinish) {
            itemState = serverSetItemResultState(data, response);
            serviceResponseHandle(response, data);
            serviceResponseToTable(manualSendRowIndex, response);
        }
        setTableItemColor(serviceSetModel, manualSendRowIndex, serviceItemStateColor[itemState]);
        manualSendRowIndex = -1;

        /* 输出提示信息 */
        QString hintMsg = QString::asprintf("<font color = %s>结束诊断服务[0x%02x],%s</font>",\
                                            serviceItemStateColor[itemState], data.SID, data.desc.toStdString().c_str());
        promptForAction(hintMsg);
    }

    if (serviceActive() && channelName == serviceActiveTask.channelName) {
        /* 计算传输速度 */
        serviceActiveTask.downTotalByte += response.size();
        transferAverageSpeed(serviceActiveTask.downPreTime, serviceActiveTask.downTotalByte, serviceActiveTask.downAverageByte);
        if (serviceActiveTask.downAverageByte / 1024 > 10) {
            downTransferSpeedLable->setText(QString("%1 KB/s").arg(serviceActiveTask.downAverageByte / 1024));
        } else {
            downTransferSpeedLable->setText(QString("%1 byte/s").arg(serviceActiveTask.downAverageByte));
        }
        /* 停止应答超时定时器 */
        serviceActiveTask.responseTimer.stop();
        serviceResponseToTable(serviceActiveTask.indexRow, response);
        serviceActiveTask.runtotal++;
        if (state == DoipClientConnect::normalFinish) {
            serviceResponseHandle(response, serviceActiveTask.data);
        }

        /* 标记响应超时 */
        if (state == DoipClientConnect::TimeoutFinish) {
            QStandardItem *item = serviceSetModel->item(serviceActiveTask.indexRow, udsViewResponseColumn);
            if (item) {
                item->setText("响应超时");
                item->setToolTip("响应超时");
            }
        }
        /* 判断当前任务是否结束，标记当前任务结果状态 */
        if (!keepCurrentTask()) {
            enum serviceItemState itemState = serviceItemTestFail;
            if (state == DoipClientConnect::normalFinish) {
                itemState = serverSetItemResultState(serviceActiveTask.data, response);
            }
            activeServiceFlicker.isvalid = false;
            if (!highPerformanceEnable) {
                setTableItemColor(serviceSetModel, serviceActiveTask.indexRow, serviceItemStateColor[itemState]);
            }
            if (itemState == serviceItemTestPass) {
                serviceItemStatis[itemState]++;
                serviceItemStatisLabel[itemState]->setText(QString("通过:%1").arg(serviceItemStatis[itemState]));
            } else if (itemState == serviceItemTestWarn) {
                serviceItemStatis[itemState]++;
                serviceItemStatisLabel[itemState]->setText(QString("警告:%1").arg(serviceItemStatis[itemState]));
            }
            else if (itemState == serviceItemTestFail) {
                serviceItemStatis[itemState]++;
                serviceItemStatisLabel[itemState]->setText(QString("失败:%1").arg(serviceItemStatis[itemState]));
            }
            serviceItemStatis[serviceItemNotTest]--;
            serviceItemStatisLabel[serviceItemNotTest]->setText(QString("未测试:%1").arg(serviceItemStatis[serviceItemNotTest]));
            if (!highPerformanceEnable) {
                serviceTestProgress->setValue(serviceItemStatis[serviceItemAll] - serviceItemStatis[serviceItemNotTest]);
            }
            /* 在表格内设置当前行的测试结果着色情况 */
            if (!highPerformanceEnable) {
                QStandardItem *resultitem = serviceSetModel->item(serviceActiveTask.indexRow, udsViewResultColumn);
                if (resultitem) {
                    resultitem->setText(serviceItemStateColor[itemState]);
                }
            }

            if (!highPerformanceEnable) {
                /* 输出提示信息 */
                serviceItem data;
                getServiceItemData(serviceActiveTask.indexRow, data);
                QString hintMsg = QString::asprintf("<font color = %s>结束诊断服务[0x%02x],%s</font>",\
                                                serviceItemStateColor[itemState], data.SID, data.desc.toStdString().c_str());
                promptForAction(hintMsg);
            }

            /* 重新开始单个计时 */
            if (!highPerformanceEnable) {
                QStandardItem *item = serviceSetModel->item(serviceActiveTask.indexRow, udsViewElapsedTimeColumn);
                if (item) {item->setText(QString("%1").arg(serviceTimer.elapsed()));}
                serviceElapsedTimeLCDNumber->display(QString("%1").arg(serviceTimer.elapsed()));
                allServiceElapsedTimeLCDNumber->display(QString("%1").arg(allServiceTimer.elapsed()));
                serviceTimer.restart();
            }

            if (failureAbortCheckBox->checkState() == Qt::Checked && \
                itemState == serviceItemTestFail) {
                serviceActiveTask.indexRow = serviceSetModel->rowCount();
                QString hintMsg = QString::asprintf("<font color = red>测试项失败，终止诊断测试</font>");
                promptForAction(hintMsg);
            }

            /* 跳转到下一个任务 */
            serviceActiveTask.indexRow++;
            serviceActiveTask.runtotal = 0;
            if (serviceActiveTask.indexRow >= serviceSetModel->rowCount()) {
                serverSetFinishHandle();
            } else {
                /* 继续执行下一个任务 */
                getServiceItemData(serviceActiveTask.indexRow, serviceActiveTask.data);
                serviceListTaskTimer->setInterval(serviceActiveTask.data.delay);
                serviceListTaskTimer->start();
            }
        }
        else {
            /* 单个任务结束 */
            if (!highPerformanceEnable) {
                allServiceElapsedTimeLCDNumber->display(QString("%1").arg(allServiceTimer.elapsed()));
                serviceElapsedTimeLCDNumber->display(QString("%1").arg(serviceTimer.elapsed()));
            }
            serviceListTaskTimer->setInterval(serviceActiveTask.data.delay);
            serviceListTaskTimer->start();
        }
    }
}

void UDS::writeServiceSetCache()
{
    saveCurrServiceSet(serviceBackCacheDir + "/serviceSetCache.json");
}

void UDS::canResponseHandler(QString name, int state, QByteArray response)
{
    qDebug() << name;
    qDebug() <<  state;
    qDebug() << response.toHex(' ');
    diagnosisResponseHandle(name, (DoipClientConnect::sendDiagFinishState)state, response);
}

void UDS::contextMenuSlot(QPoint pos)
{
    if (serviceActive()) {
        return ;
    }
    auto index = serviceSetView->indexAt(pos);
    if (index.isValid()) {
        serviceSetSelectRow = serviceSetView->rowAt(pos.y());
        serviceSetSelectCloumn = serviceSetView->columnAt(pos.x());
        serviceSetMenu->exec(QCursor::pos());
    }
}

void UDS::projectSetContextMenuSlot(QPoint pos)
{
    if (serviceActive()) {
        return ;
    }
    auto index = projectNameTable->indexAt(pos);
    if (index.isValid()) {
        projectSetSelectRow = projectNameTable->rowAt(pos.y());
        projectSetSelectCloumn = projectNameTable->columnAt(pos.x());
        projectNameWidgetMenu->exec(QCursor::pos());
        qDebug() << projectSetSelectRow << projectSetSelectCloumn;
    }
}

void UDS::serviceSetDelMenuSlot()
{

}

