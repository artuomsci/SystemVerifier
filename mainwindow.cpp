#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <assert.h>
#include <fstream>
#include <sstream>
#include <optional>

#include <QDir>
#include <QUuid>
#include <QGraphicsEllipseItem>
#include <QTimer>
#include <QKeyEvent>
#include <QFileDialog>

#include "LibBoolEE/LibBoolEE.h"

//----------------------------------------------------------------------
static const double blob_radius = 20.0;
static const QColor positive_clr(50, 200, 50, 125);
static const QColor negative_clr(200, 50, 50, 125);

//----------------------------------------------------------------------
constexpr int eUUID = Qt::UserRole + 0;

//----------------------------------------------------------------------
TDevList LoadDevList(const std::string& file_)
{
    TDevList result;

    std::ifstream file(file_);

    TDevList::iterator current_dev = result.end();

    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            auto dev_id = SDevice::IdParam(line);

            if (dev_id.has_value())
            {
                auto insert_result = result.insert( { dev_id.value(), SDevice() } );

                if (insert_result.second)
                    current_dev = insert_result.first;
                else
                {
                    Log("Error! Device id collision: " + dev_id.value());
                    current_dev = result.end();
                }
            }

            if (current_dev != result.end())
                current_dev->second.Parse_param(line);
        }

        file.close();
    }

    return result;
}

//----------------------------------------------------------------------
QGraphicsItem* GetItem(TNodeId node_id_, const QList<QGraphicsItem*>& items_list_)
{
    for (const auto& it : items_list_)
    {
        if (it->data(eUUID).toString().toStdString() == node_id_)
        {
            return it;
        }
    }
    return NULL;
}

//----------------------------------------------------------------------
void print(const QString& file_name_, const TNodeList& nodes_)
{
    std::ofstream file(file_name_.toStdString());

    // Counting occurencies
    std::map<TDevId, int> occurencies;
    for (const auto& it : nodes_)
    {
        const SDevice& dev = it.second;
        auto itDev = occurencies.find(dev.id);
        if (itDev != occurencies.end())
            itDev->second++;
        else
            occurencies[dev.id] = 1;
    }

    // Printing devices
    int i = 0;
    for (const auto& it : nodes_)
    {
        const SDevice& dev = it.second;

        auto itDev = occurencies.find(dev.id);
        if (itDev != occurencies.end())
        {
            const int item_count = itDev->second;

            file << "//============ Device #" + std::to_string(++i) << std::endl;
            file << "Items: " + std::to_string(item_count) << std::endl;
            file << dev.Print_description() << std::endl;
            occurencies.erase(itDev);
        }
    }

    file.close();
}

//----------------------------------------------------------------------
void check_states(const QList<class QGraphicsItem*>& qitems_, const TNodeList& nodes_)
{
    for (const auto& itNode : nodes_)
    {
        const TNodeId& node_id  = itNode.first;
        const SDevice& dev      = itNode.second;

        LibBoolEE::Vals vals;

        for (int i = 0; i < (int)dev.inputs.size(); ++i)
        {
            bool state  = dev.inputs[i].IsOn();

            auto symbol = ind2let(i + 1);

            CheckLog(symbol, "void check_states(...)");

            vals.insert(std::pair<std::string, bool>(std::string(1, symbol.value()), state));
        }

        QAbstractGraphicsShapeItem* pItem =
                static_cast<QAbstractGraphicsShapeItem*>(GetItem(node_id, qitems_));
        if (pItem)
        {
            if (!LibBoolEE::resolve(dev.rule, vals))
                pItem->setBrush(QBrush(negative_clr));
            else
                pItem->setBrush(QBrush(positive_clr));
        }
    }
}

//----------------------------------------------------------------------
MainWindow::MainWindow(QWidget* pParent_) :
    QMainWindow (pParent_           )
  , ui          (new Ui::MainWindow )
{
    ui->setupUi(this);

    m_root_folder = "../";
    m_data_folder = m_root_folder + "data/";

    read_categories();

    QGraphicsScene* pScene = new QGraphicsScene;

    ui->View->setScene(pScene);
    ui->View->show();

    connect(pScene, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
    connect(pScene, SIGNAL(changed(QList<QRectF>)), this, SLOT(on_scene_changed(QList<QRectF>)));

    QTimer* pTimer = new QTimer(this);
    connect(pTimer, SIGNAL(timeout()), this, SLOT(checkStates()));
    pTimer->start(1000);
}

//----------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}

//----------------------------------------------------------------------
QGraphicsEllipseItem* MainWindow::create_vis_node(const TNodeId& uuid_, const std::string& dev_name_)
{
    static const QPen   pen  (QColor(Qt::lightGray), Qt::SolidLine   );
    static const QBrush brush(QColor(Qt::lightGray), Qt::SolidPattern);

    QGraphicsEllipseItem* pItem = ui->View->scene()->addEllipse(0, 0, blob_radius, blob_radius, pen, brush);

    pItem->setFlag(QGraphicsItem::ItemIsSelectable);
    pItem->setFlag(QGraphicsItem::ItemIsMovable);
    pItem->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    pItem->setData(eUUID, QVariant(uuid_.c_str()));
    pItem->setZValue(1);

    QGraphicsTextItem* pText = ui->View->scene()->addText(dev_name_.c_str());
    pText->setParentItem(pItem);

    return pItem;
}

//----------------------------------------------------------------------
void MainWindow::on_pbAdd_clicked()
{
    std::string dev_name = ui->devList->currentIndex().data(Qt::DisplayRole).toString().toStdString();
    if (dev_name.empty())
        return;

    TCategory cat = ui->cbCategories->currentText().toStdString();

    TNodeId uuid = QUuid::createUuid().toString().toStdString();

    TDevId dev_id = ui->devList->currentIndex().data(eUUID).toString().toStdString();

    m_nodes[uuid] = m_category_list[cat][dev_id];

    create_vis_node(uuid, dev_name);
}

//----------------------------------------------------------------------
void MainWindow::on_devList_clicked(const QModelIndex &index)
{
    Q_UNUSED(index);

    TDevList list = m_category_list[ui->cbCategories->currentText().toStdString()];

    TDevId dev_id = ui->devList->currentIndex().data(eUUID).toString().toStdString();

    SDevice dev = list[dev_id];

    ui->detailsList->clear();
    ui->detailsList->addItem(dev.Print_description().c_str());
}

//----------------------------------------------------------------------
void MainWindow::on_cbCategories_activated(const QString& arg1)
{
    Q_UNUSED(arg1);
    update_dev_list();
}

//----------------------------------------------------------------------
void MainWindow::selectionChanged()
{
    QList<QGraphicsItem*> items = ui->View->scene()->selectedItems();

    enum {
        nothing_selected    = 0,
        one_item_selected   = 1,
        two_items_selected  = 2
    };

    switch (items.size()) {

    case nothing_selected:
    {
        ui->linputs->clear();
        ui->l_label->setText("---");
        m_ldev.clear();

        ui->rinputs->clear();
        ui->r_label->setText("---");
        m_rdev.clear();
    }
        break;

    case one_item_selected:
    {
        m_ldev = items[0]->data(eUUID).toString().toStdString();

        const SDevice& dev = m_nodes[m_ldev];

        ui->linputs->clear();
        for (const auto& it : dev.inputs)
        {
            ui->linputs->addItem(it.name.c_str());
            QColor clr = it.IsOn() ? QColor(Qt::yellow) : QColor(Qt::lightGray);
            ui->linputs->item(ui->linputs->count() - 1)->setBackground(QBrush(clr));
        }

        ui->linputs->update();
        ui->l_label->setText(std::string(dev.name + "\n" + dev.rule).c_str());
    }
        break;

    case two_items_selected:
    {
        {
            m_ldev = items[0]->data(eUUID).toString().toStdString();

            const SDevice& dev = m_nodes[m_ldev];

            ui->linputs->clear();
            for (const auto& it : dev.inputs)
            {
                ui->linputs->addItem(it.name.c_str());
                QColor clr = it.IsOn() ? QColor(Qt::yellow) : QColor(Qt::lightGray);
                ui->linputs->item(ui->linputs->count() - 1)->setBackground(QBrush(clr));
            }

            ui->linputs->update();
            ui->l_label->setText(std::string(dev.name + "\n" + dev.rule).c_str());
        }

        {
            m_rdev = items[1]->data(eUUID).toString().toStdString();

            const SDevice& dev = m_nodes[m_rdev];

            ui->rinputs->clear();
            for (const auto& it : dev.inputs)
            {
                ui->rinputs->addItem(it.name.c_str());
                QColor clr = it.IsOn() ? QColor(Qt::yellow) : QColor(Qt::lightGray);
                ui->rinputs->item(ui->rinputs->count() - 1)->setBackground(QBrush(clr));
            }

            ui->rinputs->update();
            ui->r_label->setText(std::string(dev.name + "\n" + dev.rule).c_str());
        }
    }
        break;

    default:
        assert(false);
        break;
    }
}

//----------------------------------------------------------------------
void MainWindow::checkStates()
{
    check_states(ui->View->scene()->items(), m_nodes);

    double power = std::accumulate(m_nodes.begin(), m_nodes.end(), 0.0, [](double init_, const TNodeList::value_type& it_) {
        return init_ + it_.second.power;
    });

    ui->lbInfo->setText("Power: " + QString::number(power) + " W");
}

//----------------------------------------------------------------------
void MainWindow::keyPressEvent(QKeyEvent* event)
{
    QMainWindow::keyPressEvent(event);

    if (event->key() == Qt::Key_Delete)
        on_pbDel_clicked();

    if (event->key() == Qt::Key_B)
        on_pbBind_clicked();

    if (event->key() == Qt::Key_U)
        on_pbUnbind_clicked();
}

//----------------------------------------------------------------------
void MainWindow::read_categories()
{
    QDir dir(m_data_folder.c_str());

    QList<QString> flList;
    for (const auto& it : dir.entryInfoList())
        if (!it.baseName().isEmpty())
            flList.append(it.baseName());

    m_category_list.clear();

    for (const auto& it : flList)
        m_category_list[it.toStdString()] = LoadDevList(m_data_folder + it.toStdString());

    ui->cbCategories    ->clear();
    ui->cbCategoriesEdit->clear();

    for (const auto& it : flList)
    {
        ui->cbCategories    ->addItem(it);
        ui->cbCategoriesEdit->addItem(it);
    }

    update_dev_list();

    m_ports.clear();

    std::set<std::string> set_test;

    auto it_connect = m_category_list.find("connections");
    if (it_connect != m_category_list.end())
    {
        const TDevList& dev_list = it_connect->second;

        for (const auto& it_dev : dev_list)
        {
            const SDevice& dev = it_dev.second;

            for (const auto& it_input : dev.inputs)
            {
                TPair pr = Split(it_input.name, ":");
                m_ports.insert(pr.second);
            }
        }
    }
}

//----------------------------------------------------------------------
void MainWindow::update_dev_list()
{
    TCategory category = ui->cbCategories->currentText().toStdString();

    auto it = m_category_list.find(category);
    if (it != m_category_list.end())
    {
        ui->devList->clear();

        const TDevList& list = it->second;
        for (auto itDev : list)
        {
            ui->devList->addItem(itDev.second.name.c_str());
            ui->devList->item(ui->devList->count() - 1)->setData(eUUID, QVariant(itDev.first.c_str()));
        }
    }
}

//----------------------------------------------------------------------
void MainWindow::restore()
{
    auto file_name = QFileDialog::getOpenFileName(this, tr("Open Scheme"), "", tr("Scheme Files (*.sch)"));

    clear();

    using reader = nop::StreamReader<std::stringstream>;

    std::string data;

    std::ifstream file(file_name.toStdString());
    if (!file.is_open())
        return;

    file.seekg(0, std::ios::end);
    data.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    data.assign((std::istreambuf_iterator<char>(file)),
                 std::istreambuf_iterator<char>());

    nop::Deserializer<reader> deserializer { data };

    deserializer.Read(&m_nodes);
    deserializer.Read(&m_links);

    for (const auto& it : m_links)
    {
        const TLinkId& uuid = it.first;

        QPen pen(QColor(Qt::lightGray), Qt::SolidLine);

        QGraphicsLineItem* pItem = ui->View->scene()->addLine(0, 0, 0, 0, pen);

        pItem->setData(eUUID, QVariant(uuid.c_str()));
    }

    for (const auto& it : m_nodes)
    {
        const TNodeId& uuid = it.first;
        const SDevice& dev = it.second;

        QGraphicsEllipseItem* pItem = create_vis_node(uuid, dev.name);

        pItem->setPos(dev.gnode.x, dev.gnode.y);
    }

    on_scene_changed(QList<QRectF>());
}

//----------------------------------------------------------------------
void MainWindow::store() const
{
    QString fileName = QFileDialog::getSaveFileName(const_cast<MainWindow*>(this), tr("Save Scheme"), "", tr("Scheme Files (*.sch)"));
    fileName = fileName.contains(".sch") ? fileName : fileName + ".sch";

    using writer = nop::StreamWriter<std::stringstream>;
    nop::Serializer<writer> serializer;

    serializer.Write(m_nodes);
    serializer.Write(m_links);

    std::string out = serializer.writer().stream().str();

    std::ofstream file(fileName.toStdString());
    file << out;
    file.close();
}

//----------------------------------------------------------------------
void MainWindow::clear()
{
    while (!m_nodes.empty())
    {
        auto it = m_nodes.begin();

        const TNodeId& id = it->first;
        if (!id.empty())
        {
             if (auto item = GetItem(id, ui->View->scene()->items()))
                 ui->View->scene()->removeItem(item);
        }

        m_nodes.erase(it);
    }

    while (!m_links.empty())
    {
        auto it = m_links.begin();

        const TLinkId& id = it->first;
        if (!id.empty())
        {
            if (auto item = GetItem(id, ui->View->scene()->items()))
                ui->View->scene()->removeItem(item);
        }

        m_links.erase(it);
    }

    m_rdev.clear();
    m_ldev.clear();

    on_scene_changed(QList<QRectF>());
}

//----------------------------------------------------------------------
void MainWindow::on_pbBind_clicked()
{
    auto lind = ui->linputs->currentRow();
    auto rind = ui->rinputs->currentRow();

    if (m_nodes.empty() || m_ldev.empty() || m_rdev.empty() || (lind == -1) || (rind == -1))
        return;

    SDevice& ldev = m_nodes[m_ldev];
    SDevice& rdev = m_nodes[m_rdev];

    assert(m_ldev != m_rdev);

    TPair linput = Split(ldev.inputs[lind].name, ":");
    TPair rinput = Split(rdev.inputs[rind].name, ":");

    for (const auto &itDev : m_category_list["connections"])
    {
        const SDevice& dev = itDev.second;

        {
            auto cn_linput = Split(dev.inputs[0].name, ":").second;
            auto cn_rinput = Split(dev.inputs[1].name, ":").second;

            if (((cn_linput == linput.second) && (cn_rinput == rinput.second)) ||
                ((cn_rinput == linput.second) && (cn_linput == rinput.second)))
            {
                if (!ldev.inputs[lind].IsOn() && !rdev.inputs[rind].IsOn())
                {
                    TLinkId uuid = QUuid::createUuid().toString().toStdString();

                    ldev.inputs[lind].connect.node  = m_rdev;
                    ldev.inputs[lind].connect.input = rind;
                    ldev.inputs[lind].connect.link  = uuid;

                    rdev.inputs[rind].connect.node  = m_ldev;
                    rdev.inputs[rind].connect.input = lind;
                    rdev.inputs[rind].connect.link  = uuid;

                    SLink& link = m_links[uuid];
                    link.nodes[0] = m_ldev;
                    link.nodes[1] = m_rdev;

                    auto litem = GetItem(m_ldev, ui->View->scene()->items());
                    auto ritem = GetItem(m_rdev, ui->View->scene()->items());

                    QPen pen(QColor(Qt::lightGray), Qt::SolidLine);

                    auto lpos = litem->boundingRect().center() + litem->pos();
                    auto rpos = ritem->boundingRect().center() + ritem->pos();

                    QGraphicsLineItem* pItem = ui->View->scene()->addLine(
                                lpos.x(), lpos.y(),
                                rpos.x(), rpos.y(),
                                pen);

                    pItem->setData(eUUID, QVariant(uuid.c_str()));
                }

                break;
            }
        }
    }
}

//----------------------------------------------------------------------
void MainWindow::on_pbDel_clicked()
{
    QList<QGraphicsItem*> items = ui->View->scene()->selectedItems();

    for (const auto& itItem : items)
    {
        const TNodeId& id = itItem->data(eUUID).toString().toStdString();
        if (id.empty())
            continue;

        for (auto& itInput : m_nodes[id].inputs)
        {
            // Removing links
            if (!itInput.connect.link.empty())
            {
                if (auto item = GetItem(itInput.connect.link, ui->View->scene()->items()))
                    ui->View->scene()->removeItem(item);
            }

            // Resetting inputs
            if (!itInput.connect.node.empty())
            {
                auto itNode = m_nodes.find(itInput.connect.node);
                if (itNode != m_nodes.end())
                    itNode->second.inputs[itInput.connect.input].connect.Reset();
            }
        }

        m_nodes.erase(id);

        ui->View->scene()->removeItem(itItem);
    }
}

//----------------------------------------------------------------------
void MainWindow::on_pbUnbind_clicked()
{
    auto lind = ui->linputs->currentRow();
    auto rind = ui->rinputs->currentRow();

    if (m_nodes.empty() || m_ldev.empty() || m_rdev.empty() || (lind == -1) || (rind == -1))
        return;

    SDevice& ldev = m_nodes[m_ldev];
    SDevice& rdev = m_nodes[m_rdev];

    // Removing links
    auto link = ldev.inputs[lind].connect.link;
    if (!link.empty())
    {
        if (auto item = GetItem(link, ui->View->scene()->items()))
            ui->View->scene()->removeItem(item);

        if (m_links.count(link))
            m_links.erase(link);
    }

    ldev.inputs[lind].connect.Reset();
    rdev.inputs[rind].connect.Reset();
}

//----------------------------------------------------------------------
void MainWindow::on_scene_changed(const QList<QRectF>& list_)
{    
    Q_UNUSED(list_);

    for (const auto& it : m_links)
    {
        if (QGraphicsLineItem* pLine  = static_cast<QGraphicsLineItem*>(GetItem(it.first, ui->View->scene()->items())))
        {
            auto litem = GetItem(it.second.nodes[0], ui->View->scene()->items());
            auto ritem = GetItem(it.second.nodes[1], ui->View->scene()->items());

            if (litem && ritem)
            {
                auto lpos = litem->boundingRect().center() + litem->pos();
                auto rpos = ritem->boundingRect().center() + ritem->pos();

                pLine->setLine(lpos.x(), lpos.y(),
                               rpos.x(), rpos.y());
            }
        }
    }

    for (const auto& itItem : ui->View->scene()->items())
    {
        TNodeId id = itItem->data(eUUID).toString().toStdString();
        if (!id.empty())
        {
            auto itNode = m_nodes.find(id);
            if (itNode != m_nodes.end())
            {
                SGraphNode& node = itNode->second.gnode;
                node.x = itItem->pos().x();
                node.y = itItem->pos().y();
            }
        }
    }

    ui->View->update();
}

//----------------------------------------------------------------------
void MainWindow::on_pbSave_clicked()
{
    store();
}

//----------------------------------------------------------------------
void MainWindow::on_pbLoad_clicked()
{
    restore();
}

//----------------------------------------------------------------------
void MainWindow::on_pbClear_clicked()
{
    clear();
}

//----------------------------------------------------------------------
void MainWindow::on_devList_itemSelectionChanged()
{
    on_devList_clicked(ui->devList->currentIndex());
}

//----------------------------------------------------------------------
void MainWindow::on_pbPrint_clicked()
{
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save configuration"), "", tr("Configuration File (*.txt)"));
    print(fileName.contains(".txt") ? fileName : fileName + ".txt", m_nodes);
}

//----------------------------------------------------------------------
void MainWindow::on_pbNewDevice_clicked()
{
    TCategory cat = ui->cbCategoriesEdit->currentText().toStdString();

    std::ofstream file; file.open(m_data_folder + cat, std::ios::ate | std::ios::app);
    if (file.is_open())
    {
        TNodeId uuid = QUuid::createUuid().toString().toStdString();

        SDevice dev;
        dev.id = shrink_str(uuid);

        QStringList descr = ui->teDevDescr->toPlainText().split('\n');
        for (const auto& it : descr)
        {
            dev.Parse_param(it.toStdString());
        }

        if (dev.Validate(m_ports))
        {
            file << "\n";
            file << dev.Storage_description();
            file << "\n";
        }
        else
        {
            Log("Bad device description!");
        }

        file.close();

        read_categories();
    }
    else Log("Couldn't open file: " + m_data_folder + cat);
}

//----------------------------------------------------------------------
void MainWindow::on_pbSaveImage_clicked()
{
    const QString fileName = QFileDialog::getSaveFileName(this, tr("Save configuration"), "", tr("Image File (*.png)"));

    ui->View->grab().save(fileName.contains(".png") ? fileName : fileName + ".png");
}

//----------------------------------------------------------------------
void MainWindow::on_pbAddCategory_clicked()
{
    TCategory cat = ui->leCategoryName->text().toStdString();
    std::ofstream file; file.open(m_data_folder + to_lower(cat));
    if (file.is_open())
    {
        file << "\n";
        file.close();

        read_categories();
    }
}
