#ifndef DIAGPCAP_H
#define DIAGPCAP_H

#include <QThread>
#include <QHostAddress>
#include <QDir>
#include <QString>
#ifdef __HAVE_PCAP__
#include <pcap.h>
#include <remote-ext.h>
#include <Iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#endif /* __HAVE_PCAP__ */

void realTimeLog(QString log);

class DiagPcap : public QThread
{
    Q_OBJECT
public:
    DiagPcap();
    bool startIPCapturePackets(QHostAddress addr);
    bool stopIPCapturePackets();
    void setCapturePacketsName(QString name);
    void run();
private:
    bool isstart = false;
    QHostAddress adaddr;
#ifdef __HAVE_PCAP__
    pcap_t *adhandle = nullptr;
#endif /* __HAVE_PCAP__ */
    QString pcapFileDir = QDir::currentPath() + "/IPpcapfile/";
    QString capturePacketsName;
};

#endif // DIAGPCAP_H
