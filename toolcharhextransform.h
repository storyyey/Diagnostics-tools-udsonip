#ifndef TOOLCHARHEXTRANSFORM_H
#define TOOLCHARHEXTRANSFORM_H

#include <QWidget>

namespace Ui {
class ToolCharHexTransform;
}

class ToolCharHexTransform : public QWidget
{
    Q_OBJECT

public:
    explicit ToolCharHexTransform(QWidget *parent = nullptr);
    ~ToolCharHexTransform();

private slots:
    void on_hexToCharBtn_clicked();

    void on_charToHexBtn_clicked();

    void on_clearPushButton_clicked();

    void on_hexPlainTextEdit_textChanged();

    void on_charPlainTextEdit_textChanged();

    void on_pushButton_clicked();

private:
    Ui::ToolCharHexTransform *ui;
    bool hexischange = false;
};

#endif // TOOLCHARHEXTRANSFORM_H
