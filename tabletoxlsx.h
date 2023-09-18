#ifndef TABLETOXLSX_H
#define TABLETOXLSX_H

#include <QObject>
#include <QStandardItemModel>

class TableToXlsx : public QObject
{
    Q_OBJECT
public:
    explicit TableToXlsx(QObject *parent = nullptr);
    bool modelToXlsx(QStandardItemModel &model, const QString &file);
    bool modelToXlsx(QStandardItemModel &model, const QString &file, int colorColumn);
    bool modelToXlsx(QStandardItemModel &model, const QString &file, const QString prefix, int colorColumn);
signals:

};

#endif // TABLETOXLSX_H
