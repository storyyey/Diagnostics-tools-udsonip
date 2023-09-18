#ifndef COMMONDB_H
#define COMMONDB_H

#include <QObject>

class CommonDB : public QObject
{
    Q_OBJECT
public:
    explicit CommonDB(QObject *parent = nullptr);

signals:

};

#endif // COMMONDB_H
