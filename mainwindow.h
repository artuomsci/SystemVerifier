#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <vector>
#include <string>
#include <map>
#include <set>

#include "nop/serializer.h"
#include "nop/structure.h"
#include "nop/utility/stream_writer.h"
#include "nop/utility/stream_reader.h"

namespace Ui {
    class MainWindow;
}

class QGraphicsEllipseItem;

//----------------------------------------------------------------------
typedef std::string TDevId;
typedef std::string TNodeId;
typedef std::string TLinkId;
typedef std::string TCategory;

//----------------------------------------------------------------------

typedef std::pair<std::string, std::string> TPair;

//----------------------------------------------------------------------
inline void Log(const std::string& msg_)
{
    printf("%s \n", msg_.c_str());
}

//----------------------------------------------------------------------
template<typename T>
const std::optional<T>& CheckLog(const std::optional<T>& val_, const std::string& msg_)
{
    if (!val_.has_value())
        printf("Error: no value. \n %s \n", msg_.c_str());
    return val_;
}

//----------------------------------------------------------------------
inline std::string shrink_str(std::string str_)
{
    if (str_.size() > 1)
    {
        str_.erase(0, 1);
        str_.erase(str_.size() - 1, 1);
    }

    return str_;
}

//----------------------------------------------------------------------
inline std::optional<char> ind2let(int n_)
{
    return n_ >= 1 && n_ <= 26 ? "abcdefghijklmnopqrstuvwxyz"[n_ - 1] : std::optional<char>();
}

//----------------------------------------------------------------------
inline TPair Split(const std::string& str_, const char* symbol_)
{
    TPair result;

    auto pos = str_.find(symbol_);
    if (pos != std::string::npos)
    {
        result.first    = str_.substr(0, pos);
        result.second   = str_.substr(pos + 1, str_.size());
    }

    return result;
}

//----------------------------------------------------------------------
inline std::string to_lower(std::string string_)
{
    std::locale loc;
    for (auto& it : string_)
        it = std::tolower(it, loc);
    return string_;
}

//----------------------------------------------------------------------
struct SConnect
{
    TNodeId node;
    TLinkId link;
    int     input { -1 };

    void Reset()
    {
        node.clear();
        link.clear();
        input = -1;
    }

    NOP_STRUCTURE(SConnect, node, link, input);
};

//----------------------------------------------------------------------
struct SInput
{
    std::string name;
    SConnect    connect;

    bool IsOn() const { return !connect.node.empty(); }

    SInput() {}
    SInput(const std::string& name_) : name(name_)
    {}

    NOP_STRUCTURE(SInput, name, connect);
};

//----------------------------------------------------------------------
struct SGraphNode
{
    int x {}, y {};

    NOP_STRUCTURE(SGraphNode, x, y);
};

//----------------------------------------------------------------------
typedef std::set<std::string> TPortList;

//----------------------------------------------------------------------
struct SDevice
{
    TDevId              id;
    std::string         name;
    std::vector<SInput> inputs;
    std::string         rule;
    SGraphNode          gnode;
    double              power   {};

    NOP_STRUCTURE(SDevice, id, name, inputs, rule, gnode, power);

    void Reset() {
        id      .clear();
        name    .clear();
        inputs  .clear();
        rule    .clear();
        gnode.x = 0;
        gnode.y = 0;
        power = 0.0;
    }

    std::string Print_description() const
    {
        std::string descr;
        descr += "Device name: " + name + "\n";
        descr += "Device I/O \n";
        descr += "//============ \n";

        for (const auto& itInput : inputs)
            descr += itInput.name + "\n";

        descr += "//============ \n";

        return descr;
    }

    std::string Storage_description() const
    {
        std::string descr;

        descr += "id:"      + id    + "\n";
        descr += "name:"    + name  + "\n";

        for (const auto& itInput : inputs)
            descr += "input:" + itInput.name + "\n";

        descr += "pwr:"     + std::to_string(power) + "\n";
        descr += "rule:"    + rule;

        return descr;
    }

    static std::optional<std::string> IdParam(const std::string& str_)
    {
        TPair pr = Split(str_, ":");

        return pr.first == "id" ? std::optional<std::string>(pr.second) : std::optional<std::string>();
    }

    void Parse_param(const std::string& str_)
    {
        TPair pair = Split(str_, ":");

        if      (pair.first == "id")
        {
            id = pair.second;
        }
        else if (pair.first == "name")
        {
            name = pair.second;
        }
        else if (pair.first == "input")
        {
            inputs.push_back(SInput(to_lower(pair.second)));
        }
        else if (pair.first == "rule")
        {
            rule = to_lower(pair.second);
        }
        else if (pair.first == "pwr")
        {
            power = std::stod(pair.second);
        }
    }

    bool Validate(const TPortList& ports_)
    {
        if (inputs.size() >= 26)
            return false;

        for (const auto& it : inputs)
        {
            if (!ports_.count(Split(it.name, ":").second))
            return false;
        }
        return true;
    }
};

typedef std::map<TDevId     , SDevice   >  TDevList;
typedef std::map<TNodeId    , SDevice   >  TNodeList;
typedef std::map<TCategory  , TDevList  >  TCategoryList;

//----------------------------------------------------------------------
struct SLink
{
    std::array<TNodeId, 2> nodes;

    NOP_STRUCTURE(SLink, nodes);
};

typedef std::map<TLinkId, SLink> TLinkList;

//----------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* pParent_ = 0);
    ~MainWindow();

private slots:
    void on_pbAdd_clicked();
    void on_devList_clicked(const QModelIndex& index);
    void on_cbCategories_activated(const QString& arg1);
    void selectionChanged();
    void on_pbBind_clicked();
    void on_pbDel_clicked();
    void on_pbUnbind_clicked();
    void on_scene_changed(const QList<QRectF>& list_);
    void on_pbSave_clicked();
    void on_pbLoad_clicked();
    void on_pbClear_clicked();
    void on_devList_itemSelectionChanged();
    void on_pbPrint_clicked();
    void on_pbNewDevice_clicked();
    void on_pbSaveImage_clicked();
    void on_pbAddCategory_clicked();

public slots:
    void checkStates();

private:

    void keyPressEvent(QKeyEvent* event);
    QGraphicsEllipseItem* create_vis_node(const TNodeId& uuid_, const std::string& dev_name_);

    void read_categories();
    void update_dev_list();
    void restore();
    void store() const;
    void clear();

    Ui::MainWindow* ui;
    TNodeList       m_nodes;
    TLinkList       m_links;
    TCategoryList   m_category_list;
    TPortList       m_ports;

    std::string     m_root_folder;
    std::string     m_data_folder;

    TNodeId         m_rdev;
    TNodeId         m_ldev;
};

#endif // MAINWINDOW_H
