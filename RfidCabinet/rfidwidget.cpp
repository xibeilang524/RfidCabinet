#include "rfidwidget.h"
#include "ui_rfidwidget.h"
#include <QPainter>
#include <QDebug>
#include <QTableWidget>
#include <QMessageBox>
#include <QHeaderView>
#include <QSettings>

#include "config.h"

RfidWidget::RfidWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RfidWidget)
{
    ui->setupUi(this);
    menu = NULL;
    flagReg = false;
    rowCount = 1;
    colCount = 1;
    spanX = -1;
    spanY = -1;
    repManager = RepertoryManager::manager();
    screenPos = QPoint(-1,-1);
    needSelScreen = true;

    cabManager = CabinetManager::manager();
    epcManager = EpcManager::manager();
    epcManager->initEpcHash();

    initMenu();
    initCabType(QString(CAB_TYPE).split("#"));
    //创建柜格布局
    creatRfidCells();
    //检查柜子是否注册
    checkCabinetId();
}

RfidWidget::~RfidWidget()
{
    delete ui;
}

void RfidWidget::initData()
{
    readCellsData();
}

void RfidWidget::goodsIn(QList<GoodsInfo *> listStore)
{
    GoodsInfo* info;
    foreach(info, listStore)
    {
        if(!checkPos(info->pos))
        {
            qDebug()<<"[goodsIn]"<<"goods pos out of range.";
        }
        RfidArea* cell = listCabinet[info->pos.x()]->areaAt(info->pos.y());

//        repManager->rfidIn(info);
        cell->updateInfo();
    }
}

void RfidWidget::goodsOut(QList<GoodsInfo *>listFetch)
{
    GoodsInfo* info;
    foreach(info, listFetch)
    {
        if(!checkPos(info->pos))
        {
            qDebug()<<"[goodsIn]"<<"goods pos out of range.";
        }
        RfidArea* cell = listCabinet[info->pos.x()]->areaAt(info->pos.y());

//        repManager->rfidOut(info);
        cell->updateInfo();
    }
}

void RfidWidget::rfidIn(QList<rfidChangeInfo *> listStore)
{
    rfidChangeInfo* info;
    QList<rfidChangeInfo *> list_store;

    foreach(info, listStore)
    {
        RfidAnt* ant = antsMap.value(info->antId, NULL);
        if(ant == NULL)
            return;

        if(ant->addId(QString(info->rfid.toHex())))
        {
//            info->pos.setX(ant->getPos().x());
//            info->pos.setY(ant->getPos().y());
            info->pos = ant->getPos();
            list_store<<info;
        }
    }
    if(!list_store.isEmpty())
        emit rfidStoreReq(list_store);
}

void RfidWidget::rfidOut(QList<rfidChangeInfo *> listFetch)
{
    rfidChangeInfo* info;
    QList<rfidChangeInfo *> list_fetch;
    qDebug()<<"[RfidWidget::rfidOut]";

    foreach(info, listFetch)
    {
        RfidAnt* ant = antsMap.value(info->antId, NULL);
        if(ant == NULL)
            return;

        if(ant->removeId(QString(info->rfid.toHex())))
        {
//            info->pos.setX(ant->getPos().x());
//            info->pos.setY(ant->getPos().y());
            info->pos = ant->getPos();
            list_fetch<<info;
        }
    }

    if(!list_fetch.isEmpty()){
        emit rfidFetchReq(list_fetch);
    }
}

void RfidWidget::rfidTest(QString)
{
    emit doorStareChanged(true);
}

void RfidWidget::cabRegRst(bool success)
{
    if(success)
    {
        ui->idLine->setText(cabManager->getCabinetId());
        setMsgCorlor(ui->msg_reg, "rgb(50,250,50)");
        ui->msg_reg->setText("注册成功");
    }
    else
    {

    }
}

void RfidWidget::usrLogRst(UserInfo*)
{
    emit lockActive(true);
}

void RfidWidget::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void RfidWidget::showEvent(QShowEvent *)
{
    ui->serverAddr->setText(cabManager->getServerAddress());
}

void RfidWidget::initMenu()
{
    if(menu != NULL)
        return;

    menu = new QButtonGroup(this);
    menu->addButton(ui->fetch);
    menu->addButton(ui->store);

    menu->setExclusive(true);
    ui->menuStack->setCurrentIndex(0);
    setMenuPow(1);
}

void RfidWidget::initAnt()
{
    QSettings settings(FILE_CONFIG_CABINET_LAYOUT, QSettings::IniFormat);
    QStringList list_ants;

    settings.beginGroup("Ants");
    list_ants = settings.childGroups();
    if(list_ants.isEmpty())
    {
        creatDeafultAnts();
    }
    else
    {
        qDebug("initAnt");
    }
    creatAntsMap();
}

void RfidWidget::initCabType(QStringList typeList)
{
    ui->cabType->addItems(typeList);
    ui->cabType->setCurrentIndex(0);
}

void RfidWidget::initLockConfig()
{
    ui->lock_col->setMaxCount(cabManager->cabinetColCount());
    ui->lock_col->setCurrentIndex(0);
    //    ui->lock_row->setMaxCount();
}

void RfidWidget::checkCabinetId()
{
//    if(cabManager->getCabinetId().isEmpty())//柜子未注册
//    {
//        ui->widgetStack->setCurrentIndex(1);
//    }
//    else
    {
        ui->widgetStack->setCurrentIndex(0);
    }
}

QString RfidWidget::cellStyle(QColor rgb)
{
    QString ret = QString("color:rgb(255,255,255);background-color: rgb(%1, %2, %3);margin-top:5px;margin-bottom:5px;").arg(rgb.red()).arg(rgb.green()).arg(rgb.blue());
    return ret;
}

//判断pos是否超出柜格范围
bool RfidWidget::checkPos(QPoint pos)
{
    if(pos.x()>=listCabinet.count())
        return false;
    if(pos.y()>=listCabinet[pos.x()]->rowCount())
        return false;

    return true;
}

void RfidWidget::readCellsData()
{
    Cabinet* cab;
    int i=0;
    emit test();

    foreach(cab, listCabinet)
    {
        for(i=0; i<cab->rowCount(); i++)
        {
            RfidArea* area = cab->areaAt(i);
            area->readAreaData();
            area->updateInfo();
            emit insertRfidReq(area->list_id);
        }
    }
}

void RfidWidget::setMenuPow(int _pow)
{
    ui->fetch->hide();
    ui->store->hide();
    ui->cell_config->hide();
    switch(_pow){
    case 0:
        break;
    case 1:
        ui->cell_config->show();
    default:
        break;
    }
}

void RfidWidget::setMsgCorlor(QLabel *lab, QString color)
{
    if(lab == NULL)
        return;
    lab->setStyleSheet(QString("font: 16px \"微软雅黑\"; color:%1;").arg(color));
}

void RfidWidget::creatRfidCells()
{
    QSettings settings(FILE_CONFIG_CABINET_LAYOUT, QSettings::IniFormat);
    QString strLayout = settings.value("layout", QString()).toString();
    if(strLayout.isEmpty())
        return;
    listLayout = strLayout.split("#");

    QStringList strScreenPos = settings.value("screenPos", QString()).toString().split(",");
    if(strScreenPos.count() != 2)
        return;

    bool ok;
    screenPos.setX(strScreenPos.at(0).toInt(&ok));
    if(!ok)
        return;
    screenPos.setY(strScreenPos.at(1).toInt(&ok));
    if(!ok)
        return;

    if(!listCabinet.isEmpty())
    {
        qDeleteAll(listCabinet.begin(), listCabinet.end());
        listCabinet.clear();
    }

    int i=0;
    for(i=0; i<listLayout.count(); i++)
    {
        Cabinet* cab = new Cabinet(listLayout.at(i), i);
        ui->cabLayout->layout()->addWidget(cab);
        listCabinet<<cab;
    }
    needSelScreen = false;
    listCabinet[screenPos.x()]->setScreenPos(screenPos);

    initAnt();

//    int i=0;
//    int j=0;
//    for(i=0; i<listCabinet.count(); i++)
//    {
//        for(j=0; j<listCabinet.at(i)->rowCount(); j++)
//        {
//            if((i==screenPos.x()) && ( j==screenPos.y()))
//            {
//                continue;
//            }
//            RfidArea* area = new RfidArea;
//            listCabinet[i]->setCellWidget(j, 0, area);
//        }
    //    }
}

void RfidWidget::creatDeafultAnts()
{
    if(listCabinet.isEmpty())
        return;

    int antId = 0;
    int i=0;
    int j=0;
    for(i=0; i<listCabinet.count(); i++)
    {
        for(j=0; j<listCabinet.at(i)->rowCount(); j++)
        {
            RfidAnt* ant = new RfidAnt(antId, listCabinet[i]->areaAt(j)) ;
            antId++;
            listAnt<<ant;
        }
    }
}

void RfidWidget::creatAntsMap()
{
    if(listAnt.isEmpty())
        return;

    if(!antsMap.isEmpty())
        antsMap.clear();

    RfidAnt* ant;
    foreach(ant, listAnt)
    {
        antsMap.insert(ant->getAntId(), ant);
    }
}

void RfidWidget::saveCellsInfo()
{
    QSettings settings(FILE_CONFIG_CABINET_LAYOUT, QSettings::IniFormat);
    settings.setValue("layout",listLayout.join("#"));
    settings.setValue("screenPos",QString("%1,%2").arg(screenPos.x()).arg(screenPos.y()));
    settings.sync();
}

void RfidWidget::rfidCellClear()
{
    colCount = 1;
    rowCount = 1;
}

void RfidWidget::menuLock()
{

}

void RfidWidget::menuUnlock()
{

}

void RfidWidget::readCellsInfo()
{
//    QFile fLayout(FILE_CONFIG_CABINET_LAYOUT);
//    if(!fLayout.exists())
//        return;
//    fLayout.open(QFile::ReadOnly);
//    QByteArray cellsInfo = QByteArray::fromHex(fLayout.readAll());
//    qDebug()<<"[readCellsInfo]"<<cellsInfo.toHex();
//    setCellsLayout(cellsInfo);
}

void RfidWidget::on_save_clicked()
{
    saveCellsInfo();
    qDeleteAll(listTab.begin(), listTab.end());
    listTab.clear();
    creatRfidCells();
}

void RfidWidget::on_back_clicked()
{
    ui->menuStack->setCurrentIndex(0);
}

void RfidWidget::on_cell_config_clicked()
{
    ui->menuStack->setCurrentIndex(1);
}

void RfidWidget::on_clear_config_clicked()
{
    if(!listLayout.isEmpty())
        listLayout.clear();
    if(!listCabinet.isEmpty())
    {
        qDeleteAll(listCabinet.begin(), listCabinet.end());
        listCabinet.clear();
    }
    needSelScreen = true;
    screenPos.setX(-1);
    screenPos.setY(-1);
}

void RfidWidget::on_test_open_clicked(bool checked)
{
    if(checked)//开门
    {
        ui->test_open->setText("关门");
    }
    else
    {
        ui->test_open->setText("开门模拟");
    }
    emit doorStareChanged(checked);
}

//void RfidWidget::on_cabType_currentTextChanged(const QString &arg1)
//{
//    cabSplit(arg1, ui->preCab);
//}

void RfidWidget::cabSplit(QString scale, QTableWidget *table)
{
    if(scale.isEmpty()||(table == NULL))
    {
        return;
    }
    int rowCount = scale.length();
    int baseCount = getBaseCount(scale);
    int baseHeight = table->geometry().height()/baseCount;
    int i = 0;
    table->setRowCount(rowCount);
    table->setColumnCount(1);

    table->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setVisible(false);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    qDebug()<<table->geometry().height()<<baseCount<<baseHeight;
    for(i=0; i<rowCount; i++)
    {
        table->setRowHeight(i,baseHeight*(scale.mid(i,1).toInt()));
    }
}

void RfidWidget::cabGridVisible(bool show)
{
    int i=0;

    for(i=0; i<listCabinet.count(); i++)
    {
        listCabinet[i]->setShowGrid(show);
    }
}

int RfidWidget::getBaseCount(QString scale)
{
    int i = 0;
    int ret = 0;
    if(scale.isEmpty())
        return ret;

    for(i=0; i<scale.length(); i++)
    {
        ret += scale.mid(i,1).toInt();
    }
    return ret;
}

void RfidWidget::on_preCab_clicked(const QModelIndex &index)
{
    qDebug()<<index.row()<<index.column();
    if(!needSelScreen)
    {
        ui->preCab->clearSelection();
        return;
    }

    if(index.row() == screenPos.y())
    {
        ui->preCab->clearSelection();
        screenPos.setY(-1);
        screenPos.setX(-1);
        return;
    }

    screenPos.setY(index.row());
}

void RfidWidget::on_addCab_clicked()
{
    QTableWidget* tab = new QTableWidget(ui->cabLayout);
    tab->setSelectionMode(QAbstractItemView::SingleSelection);
    tab->resize(10,ui->cabLayout->geometry().height()-12);

    ui->cabLayout->layout()->addWidget(tab);
    cabSplit(ui->cabType->currentText(), tab);
    listLayout<<ui->cabType->currentText();
    listTab<<tab;

    if((screenPos.y() >= 0) && needSelScreen)//已经选择了屏幕位置
    {
        needSelScreen = false;
        screenPos.setX(listLayout.count()-1);
        QTableWidgetItem* item = new QTableWidgetItem();
        item->setBackgroundColor(QColor(62, 155, 255));
        tab->setItem(screenPos.y(),0,item);
        ui->preCab->clearSelection();
        ui->warnSrePos->setVisible(false);
    }
}

void RfidWidget::on_delCab_clicked()
{
    if(listTab.isEmpty() || listLayout.isEmpty())
        return;

    QTableWidget* tab = listTab.takeLast();
    ui->cabLayout->layout()->removeWidget(tab);
    delete tab;
    tab = NULL;
    listLayout.removeLast();
    if(listTab.count() <= screenPos.x())
    {
        needSelScreen = true;
        screenPos.setX(-1);
        screenPos.setY(-1);
        ui->warnSrePos->setVisible(true);
    }
}

void RfidWidget::on_cabType_currentIndexChanged(const QString &arg1)
{
    cabSplit(arg1, ui->preCab);
}

void RfidWidget::on_serverAddr_editingFinished()
{
    QString str = ui->serverAddr->text();

    if(str.split('.').count() > 4)
    {
        int index = str.lastIndexOf('.');
        str.replace(index,1,':');
        ui->serverAddr->setText(str);
    }
}

void RfidWidget::on_save_settings_clicked()
{
    ui->widgetStack->setCurrentIndex(0);
}

void RfidWidget::on_set_clicked()
{
    ui->widgetStack->setCurrentIndex(1);
    initLockConfig();
}

void RfidWidget::on_cabReg_clicked()
{
    emit cabRegReq();
}

void RfidWidget::on_set_server_addr_clicked()
{
    ui->msg_server_addr->clear();
    if(!cabManager->setServerAddress(ui->serverAddr->text()))
    {
        setMsgCorlor(ui->msg_server_addr, "rgb(250,50,50)");
        ui->msg_server_addr->setText("服务器地址非法,应为:IP:Port");
    }
    else
    {
        setMsgCorlor(ui->msg_server_addr, "rgb(50,250,50)");
        ui->msg_server_addr->setText("保存成功");
        emit updateServerAddress();
    }
}
