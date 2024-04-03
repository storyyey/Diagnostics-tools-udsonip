#include "diagpcap.h"
#include <QDateTime>

DiagPcap::DiagPcap()
{
    QDir pcapdir(pcapFileDir);
    if (!pcapdir.exists()) {
        pcapdir.mkdir(pcapFileDir);
    }
}

#ifdef __HAVE_PCAP__
/* Callback function invoked by libpcap for every incoming packet */
void packet_handler(u_char *dumpfile, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
    /* save the packet on the dump file */
    pcap_dump(dumpfile, header, pkt_data);
}
#endif /* __HAVE_PCAP__ */

bool DiagPcap::startIPCapturePackets(QHostAddress addr)
{
    if (!this->isRunning()) {
        this->adaddr = addr;
        this->start();
        return true;
    }

    return false;
}

bool DiagPcap::stopIPCapturePackets()
{
    isstart = false;
    return true;
}

void DiagPcap::setCapturePacketsName(QString name)
{
    if (name.size() > 0) {
        this->capturePacketsName = pcapFileDir + name;
    }
}

#include <QNetworkInterface>

void DiagPcap::run()
{
    QList<QNetworkInterface> netList =  QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface net,netList)
     {
         //遍历每一个接口信息
         qDebug()<<"id:"<<net.index();//设备名称
         qDebug()<<"Device:"<<net.humanReadableName();//设备名称
         qDebug()<<"HardwareAddress:"<<net.hardwareAddress();//获取硬件地址
         QList<QNetworkAddressEntry> entryList = net.addressEntries();//获取ip地址和子网掩码和广播地址
         foreach(QNetworkAddressEntry entry,entryList)
         {//遍历ip条目列表
             qDebug()<<"IP address："<<entry.ip().toString();//获取ip
             qDebug()<<"Netmask:"<<entry.netmask().toString();//获取子网掩码
             qDebug()<<"Broadcast:"<<entry.broadcast().toString();//获取广播地址
         }
    }
#ifdef __HAVE_PCAP__
    pcap_if_t *alldevs = nullptr, *pDev = nullptr;
    pcap_addr_t *devaddr = nullptr;
    char filter_exp[] = "ip and port 13400";
    struct bpf_program fp;
    pcap_dumper_t *dumpfile = nullptr;
    char errbuf[PCAP_ERRBUF_SIZE + 1] = {0};

    realTimeLog("Hostaddr:" + adaddr.toString());    
    /* Retrieve the device list */
    if(pcap_findalldevs(&alldevs, errbuf) == -1) {
       //出错了返回自定义错误代码‘2’，通常用于定位故障;
    }
    pDev = alldevs;
    while (pDev) {
        realTimeLog(QString::asprintf("接口名：%s", pDev->name));
        realTimeLog(QString::asprintf("接口描述：%s", pDev->description));
        devaddr = pDev->addresses;
        while (devaddr) {
            // QHostAddress addr(devaddr->addr);
            realTimeLog(QString::asprintf("  接口地址：%s", QHostAddress(devaddr->addr).toString().toStdString().c_str()));
            devaddr = devaddr->next;
        }
        pDev = pDev->next;
    }

    PMIB_IPFORWARDROW pBestRoute = new MIB_IPFORWARDROW;
    DWORD dwGateway = 0, *pIPAddr = nullptr, *pNetMask = nullptr;
    realTimeLog("(adaddr.toIPv4Address():" + QHostAddress(adaddr.toIPv4Address()).toString());
    if(GetBestRoute(adaddr.toIPv4Address(), 0, pBestRoute) == NO_ERROR) {
        dwGateway = pBestRoute->dwForwardNextHop;
        realTimeLog("dwGateway:" + QHostAddress(htonl(dwGateway)).toString());
    }
    realTimeLog("GetBestRoute:" + QString("%1").arg(NO_ERROR));
    realTimeLog("dwGateway:" + QHostAddress(htonl(dwGateway)).toString());

    pcap_addr_t *a = nullptr;
    for(pDev = alldevs; pDev; pDev = pDev->next) {
        for(a = pDev->addresses ; a ; a=a->next) {
            if(a->addr->sa_family == AF_INET) {
                pIPAddr = (DWORD*)(&a->addr->sa_data[2]);
                pNetMask = (DWORD*)(&a->netmask->sa_data[2]);
                realTimeLog("pIPAddr:" + QHostAddress(htonl(*pIPAddr)).toString());
                realTimeLog("pNetMask:" + QHostAddress(htonl(*pNetMask)).toString());
                if((*pIPAddr & *pNetMask) == (dwGateway & *pNetMask)) {
                    realTimeLog(QString::asprintf("发现接口名：%s", pDev->name));
                    realTimeLog(QString::asprintf("发现接口描述：%s", pDev->description)); 
                    delete pBestRoute;
                    pBestRoute = nullptr;
                    goto OPEN_ADHANDLE;
                }
            }
        }
    }
    if (pBestRoute) {
        delete pBestRoute;
        pBestRoute = nullptr;
    }

OPEN_ADHANDLE:
    if (pDev == nullptr) {
        return ;
    }

    /* Open the adapter */
    realTimeLog(QString::asprintf("打开接口：%s", pDev->name));
    if ((adhandle= pcap_open_live(pDev->name,	// name of the device
                             65536,			// portion of the packet to capture.
                                            // 65536 grants that the whole packet will be captured on all the MACs.
                             0,				// promiscuous mode (nonzero means promiscuous)
                             1000,			// read timeout
                             errbuf			// error buffer
                             )) == NULL)
    {
        realTimeLog(QString::asprintf("Unable to open the adapter. %s is not supported by WinPcap", pDev->name));
        /* Free the device list */
        goto PCAP_FINISH;
    }

    /* Open the dump file */
    if (capturePacketsName.size() == 0) {
        capturePacketsName = pcapFileDir + adaddr.toString() + QDateTime::currentDateTime().toString("_yyyy_MM_dd_hh_mm_ss") + ".pcap";
        // capturePacketsName = adaddr.toString() + QDateTime::currentDateTime().toString("_yyyy_MM_dd_hh_mm_ss") + ".pcap";
    }
    dumpfile = pcap_dump_open(adhandle, capturePacketsName.toStdString().c_str());
    if(dumpfile == nullptr) {
        realTimeLog("Error opening output file");
        goto PCAP_FINISH;
    }

    if (pcap_compile(adhandle, &fp, filter_exp, 0, 0) == -1) {
        realTimeLog(QString::asprintf("Error compiling filter: %s", pcap_geterr(adhandle)));
        goto PCAP_FINISH;
    }
#if 0
    if (pcap_setfilter(adhandle, &fp) == -1) {
        realTimeLog(QString::asprintf("Error setting filter: %s", pcap_geterr(adhandle)));
        goto PCAP_FINISH;
    }
#endif
    isstart = true;
    while (isstart) {
        pcap_dispatch(adhandle, 0, packet_handler, (unsigned char *)dumpfile);
    }

PCAP_FINISH:
    if (adhandle) {
        pcap_close(adhandle);
        adhandle = nullptr;
    }
    if (dumpfile) {
        realTimeLog(QString::asprintf("Close pcap dump file: %s", capturePacketsName.toStdString().c_str()));
        pcap_dump_close(dumpfile);
        dumpfile = nullptr;
    }
    pcap_freealldevs(alldevs);
    this->capturePacketsName.clear();
#endif /* __HAVE_PCAP__ */
}

