#include "tabletoxlsx.h"
#include "QtXlsx/xlsxdocument.h"
#include "QtXlsx/xlsxformat.h"
#include "QtXlsx/xlsxworkbook.h"
#include "QtXlsx/xlsxworksheet.h"
#include <QDebug>

QTXLSX_USE_NAMESPACE

TableToXlsx::TableToXlsx(QObject *parent) : QObject(parent)
{

}

bool TableToXlsx::modelToXlsx(QStandardItemModel &model, const QString &file)
{
    Document xlsx(file);
    int row = 0, column = 0;
    int sheetRowCount = 0;

    xlsx.addSheet("sheet1");
    QXlsx::Workbook *workBook = xlsx.workbook();
    if (workBook) {
        QXlsx::Worksheet *workSheet = static_cast<QXlsx::Worksheet*>(workBook->sheet(0));
        if (workSheet) {
            sheetRowCount = workSheet->dimension().rowCount();
        }
    }

    if (sheetRowCount == 0) {
        for (column = 1; column <= model.columnCount() ; column++) {
            QStandardItem *item = model.horizontalHeaderItem(column - 1);
            if (item) {
                xlsx.write(1, column, item->text());
            }
        }
    }

    for (row = 2; row <= model.rowCount() + 1; row++) {
        for (column = 1; column <= model.columnCount() ; column++) {
            QStandardItem *item = model.item(row - 2, column - 1);
            if (item) {
                QXlsx::Format format;
                format.setBorderStyle(Format::BorderThin);
                xlsx.write(sheetRowCount + row, column, item->text(), format);
            }
        }
    }
    xlsx.save();

    return 0;
}

bool TableToXlsx::modelToXlsx(QStandardItemModel &model, const QString &file, int colorColumn)
{
    Document xlsx(file);
    int row = 0, column = 0;
    int sheetRowCount = 0;

    xlsx.addSheet("sheet1");
    QXlsx::Workbook *workBook = xlsx.workbook();
    if (workBook) {
        QXlsx::Worksheet *workSheet = static_cast<QXlsx::Worksheet*>(workBook->sheet(0));
        if (workSheet) {
            sheetRowCount = workSheet->dimension().rowCount();
        }
    }

    if (sheetRowCount == 0) {
        for (column = 1; column <= model.columnCount() ; column++) {
            QStandardItem *item = model.horizontalHeaderItem(column - 1);
            if (item) {
                xlsx.write(1, column, item->text());
            }
        }
    }

    for (row = 2; row <= model.rowCount() + 1; row++) {
        for (column = 1; column <= model.columnCount() ; column++) {
            if (column - 1 == colorColumn) {
                continue;
            }
            QStandardItem *item = model.item(row - 2, column - 1);
            if (item) {
                QXlsx::Format format;
                QStandardItem *coloritem = model.item(row - 2, colorColumn);
                if (coloritem) {
                    format.setPatternBackgroundColor(coloritem->text());//设置单元格背景色
                }
                format.setBorderColor("#000000");
                format.setBorderStyle(Format::BorderThin);
                xlsx.write(sheetRowCount + row, column, item->text(), format);
            }
        }
    }
    xlsx.save();

    return 0;
}

bool TableToXlsx::modelToXlsx(QStandardItemModel &model, const QString &file, const QString prefix, int colorColumn)
{
    Document xlsx(file); /* 打开大文件这里会耗时很长 */
    int row = 0, column = 0;
    int sheetRowCount = 0;

    xlsx.addSheet("sheet1");
    QXlsx::Workbook *workBook = xlsx.workbook();
    if (workBook) {
        QXlsx::Worksheet *workSheet = static_cast<QXlsx::Worksheet*>(workBook->sheet(0));
        if (workSheet) {
            sheetRowCount = workSheet->dimension().rowCount();
        }
    }

    if (sheetRowCount == 0) {
        for (column = 1; column <= model.columnCount() ; column++) {
            QStandardItem *item = model.horizontalHeaderItem(column - 1);
            if (item) {
                xlsx.write(1, column, item->text());
            }
        }
        sheetRowCount++;
    }

    sheetRowCount++;
    xlsx.mergeCells(CellRange(sheetRowCount, 1, sheetRowCount, model.columnCount()));
    xlsx.write(sheetRowCount, 1, prefix);
    for (row = 1; row <= model.rowCount(); row++) {
        for (column = 1; column <= model.columnCount() ; column++) {
            if (column - 1 == colorColumn) {
                continue;
            }
            QStandardItem *item = model.item(row - 1, column - 1);
            if (item) {
                QXlsx::Format format;
                QStandardItem *coloritem = model.item(row - 1, colorColumn);
                if (coloritem) {
                    format.setPatternBackgroundColor(coloritem->text());//设置单元格背景色
                }
                format.setBorderColor("#000000");
                format.setBorderStyle(Format::BorderThin);
                xlsx.write(sheetRowCount + row, column, item->text(), format);
            }
        }
    }
    xlsx.save(); /* 保存大文件这里会耗时很长 */

    return 0;
}
