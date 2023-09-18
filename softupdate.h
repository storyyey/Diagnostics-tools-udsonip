#ifndef SOFTUPDATE_H
#define SOFTUPDATE_H

#include <QThread>

#define SOFT_VERSION "3.1.7"

class SoftUpdate : public QThread
{
    Q_OBJECT
public:
    SoftUpdate();

private:

};

#endif // SOFTUPDATE_H
