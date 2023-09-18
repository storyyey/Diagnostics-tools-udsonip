#ifndef UDSPROTOCOLANALYSIS_H
#define UDSPROTOCOLANALYSIS_H

#include <QWidget>

namespace Ui {
class UDSProtocolAnalysis;
}

class UDSProtocolAnalysis : public QWidget
{
    Q_OBJECT

public:
    explicit UDSProtocolAnalysis(QWidget *parent = nullptr);
    ~UDSProtocolAnalysis();

private:
    Ui::UDSProtocolAnalysis *ui;
};

#endif // UDSPROTOCOLANALYSIS_H
