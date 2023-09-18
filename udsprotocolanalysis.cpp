#include "udsprotocolanalysis.h"
#include "ui_udsprotocolanalysis.h"

UDSProtocolAnalysis::UDSProtocolAnalysis(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UDSProtocolAnalysis)
{
    ui->setupUi(this);
}

UDSProtocolAnalysis::~UDSProtocolAnalysis()
{
    delete ui;
}
