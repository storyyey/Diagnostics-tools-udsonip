#include <QVBoxLayout>
#include "messageview.h"

MessageView::MessageView(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    this->setLayout(mainLayout);

    QHBoxLayout *toolBarLayout = new QHBoxLayout();
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setDocumentMode(true);
    tabWidget->setStyleSheet("QTabBar::tab:selected {\
                               width: 100px; \
                               height: 30px;\
                               color: #FFFFFF;\
                               background-color: #000000;\
                              } \
                              QTabBar::tab:!selected {\
                               width: 100px;\
                               height: 30px;\
                               color: #000000;\
                               background-color: #FFFFFF;\
                              }"\
                              "QTabWidget::pane {\
                                  border: 0px;\
                              }");
    mainLayout->addLayout(toolBarLayout);
    toolBarLayout->addStretch();
    UDSitemTitle << "时间" << "SA" << "TA" << "消息" << "消息长度";
    UDStabModel = new QStandardItemModel();
    UDStabModel->setColumnCount(UDSitemTitle.length());
    UDStabModel->setHorizontalHeaderLabels(UDSitemTitle);
    UDStabView = new QTableView();
    tabWidget->addTab(UDStabView, "UDS消息列表");
    UDStabView->setModel(UDStabModel);
    UDStabView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    UDStabView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    UDStabView->setAlternatingRowColors(true);
    UDStabView->setContextMenuPolicy(Qt::CustomContextMenu);
    UDStabView->setSortingEnabled(true);

    DOIPitemTitle << "描述" << "时间" << "服务端地址" << "消息" ;
    DOIPtabModel = new QStandardItemModel();
    DOIPtabModel->setColumnCount(DOIPitemTitle.length());
    DOIPtabModel->setHorizontalHeaderLabels(DOIPitemTitle);
    DOIPtabView = new QTableView();
    tabWidget->addTab(DOIPtabView, "DOIP消息列表");
    DOIPtabView->setModel(DOIPtabModel);
    DOIPtabView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    DOIPtabView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    DOIPtabView->setColumnWidth(0, 200);
    DOIPtabView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    DOIPtabView->setColumnWidth(1, 90);
    DOIPtabView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    DOIPtabView->setColumnWidth(2, 140);
    DOIPtabView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    DOIPtabView->setAlternatingRowColors(true);
    DOIPtabView->setContextMenuPolicy(Qt::CustomContextMenu);
    DOIPtabView->setSortingEnabled(true);

    mainLayout->addWidget(tabWidget);
}

void MessageView::setAddDoipMessage(bool add)
{
    isAddDoipMessage = add;
}
#include "QDebug"
void MessageView::addDoipMessage(const doipMessage_t &val)
{
    if (!isAddDoipMessage) return ;

    QList<QStandardItem *> itemList;

    if (DOIPtabModel->rowCount() >= 20000) {
        DOIPtabModel->removeRow(0);
    }

    QStandardItem *directionItem = new QStandardItem(val.direction + val.msgType);
    //directionItem->setToolTip(val.direction);
    itemList.append(directionItem);

    QStandardItem *timeItem = new QStandardItem(val.time);
    //timeItem->setToolTip(val.time);
    itemList.append(timeItem);

    QStandardItem *addrTypdeItem = new QStandardItem(QString("%1:%2").arg(val.address.toString()).arg(val.port));
    //addrTypdeItem->setToolTip(QString("%1:%2").arg(val.address.toString()).arg(val.port));
    itemList.append(addrTypdeItem);

    QStandardItem *msgItem = new QStandardItem(val.msg.size() < 50 ? QString(val.msg.toHex(' ')): val.msg.mid(0, 20).toHex(' '));
    //msgItem->setToolTip(QString(val.msg.toHex(' ')));
    itemList.append(msgItem);

    if (val.state != doipMessageNormal) {
        for (int n = 0; n < itemList.size(); n++) {
            if (val.state == doipMessageError) {
                itemList.at(n)->setBackground(QBrush("#FF0000"));
            }
            else if (val.state == doipMessageWarning) {
                itemList.at(n)->setBackground(QBrush("#FFFFCD"));
            }
        }
    }
    else {
        for (int n = 0; n < itemList.size(); n++) {
            itemList.at(n)->setBackground(QBrush("#7FFFD4"));
        }
    }

    DOIPtabModel->appendRow(itemList);
    DOIPtabView->scrollTo(DOIPtabModel->index(DOIPtabModel->rowCount() - 1, 0));
}
