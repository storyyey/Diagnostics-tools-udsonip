#ifndef TOOLREALTIMELOG_H
#define TOOLREALTIMELOG_H

#include <QWidget>

namespace Ui {
class ToolRealTimeLog;
}

class ToolRealTimeLog : public QWidget
{
    Q_OBJECT

public:
    explicit ToolRealTimeLog(QWidget *parent = nullptr);
    ~ToolRealTimeLog();
    void realtimeLog(QString log);
private:
    Ui::ToolRealTimeLog *ui;
};

#endif // TOOLREALTIMELOG_H
