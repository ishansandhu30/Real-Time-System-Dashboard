#include "process_table.h"
#include <QHeaderView>
#include <QHBoxLayout>
#include <algorithm>

// ---- ProcessTableModel ----

ProcessTableModel::ProcessTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int ProcessTableModel::rowCount(const QModelIndex& /*parent*/) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_processes.size());
}

int ProcessTableModel::columnCount(const QModelIndex& /*parent*/) const {
    return ColumnCount;
}

QVariant ProcessTableModel::data(const QModelIndex& index, int role) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!index.isValid() || index.row() >= static_cast<int>(m_processes.size()))
        return {};

    const ProcessInfo& proc = m_processes[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case PID:           return proc.pid;
            case Name:          return QString::fromStdString(proc.name);
            case CPUPercent:    return QString("%1%").arg(proc.cpu_percent, 0, 'f', 1);
            case MemoryPercent: return QString("%1%").arg(proc.memory_percent, 0, 'f', 1);
            case MemoryBytes:   return formatBytes(proc.memory_bytes);
            case Status:        return QString::fromStdString(proc.status);
            default:            return {};
        }
    }

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
            case PID:
            case CPUPercent:
            case MemoryPercent:
            case MemoryBytes:
                return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
            default:
                return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    // Sort role: return raw numeric values for proper numeric sorting
    if (role == Qt::UserRole) {
        switch (index.column()) {
            case PID:           return proc.pid;
            case Name:          return QString::fromStdString(proc.name);
            case CPUPercent:    return proc.cpu_percent;
            case MemoryPercent: return proc.memory_percent;
            case MemoryBytes:   return static_cast<qulonglong>(proc.memory_bytes);
            case Status:        return QString::fromStdString(proc.status);
            default:            return {};
        }
    }

    return {};
}

QVariant ProcessTableModel::headerData(int section, Qt::Orientation orientation,
                                        int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
        case PID:           return "PID";
        case Name:          return "Process Name";
        case CPUPercent:    return "CPU %";
        case MemoryPercent: return "MEM %";
        case MemoryBytes:   return "Memory";
        case Status:        return "Status";
        default:            return {};
    }
}

Qt::ItemFlags ProcessTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void ProcessTableModel::updateProcesses(
    const std::vector<ProcessInfo>& processes)
{
    beginResetModel();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_processes = processes;
    }
    endResetModel();
}

QString ProcessTableModel::formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double value = static_cast<double>(bytes);

    while (value >= 1024.0 && unitIndex < 4) {
        value /= 1024.0;
        ++unitIndex;
    }

    return QString("%1 %2").arg(value, 0, 'f', 1).arg(units[unitIndex]);
}

// ---- ProcessTable (composite widget) ----

ProcessTable::ProcessTable(QWidget* parent)
    : QGroupBox("Process List", parent)
{
    auto* layout = new QVBoxLayout(this);

    // Search bar and count
    auto* topLayout = new QHBoxLayout();

    m_searchBox = new QLineEdit();
    m_searchBox->setPlaceholderText("Search processes...");
    m_searchBox->setStyleSheet(
        "QLineEdit { "
        "  background: #313244; "
        "  color: #cdd6f4; "
        "  border: 1px solid #585b70; "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "}"
    );
    topLayout->addWidget(m_searchBox, 1);

    m_countLabel = new QLabel("0 processes");
    m_countLabel->setStyleSheet("color: #a6adc8;");
    topLayout->addWidget(m_countLabel);

    layout->addLayout(topLayout);

    // Table model and proxy
    m_model = new ProcessTableModel(this);
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(ProcessTableModel::Name);
    m_proxyModel->setSortRole(Qt::UserRole);

    // Table view
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_proxyModel);
    m_tableView->setSortingEnabled(true);
    m_tableView->sortByColumn(ProcessTableModel::CPUPercent, Qt::DescendingOrder);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->setMinimumHeight(200);

    m_tableView->setStyleSheet(
        "QTableView { "
        "  background: #1e1e2e; "
        "  alternate-background-color: #262637; "
        "  color: #cdd6f4; "
        "  gridline-color: #313244; "
        "  border: 1px solid #585b70; "
        "  border-radius: 4px; "
        "  selection-background-color: #45475a; "
        "  selection-color: #cdd6f4; "
        "} "
        "QHeaderView::section { "
        "  background: #313244; "
        "  color: #cdd6f4; "
        "  padding: 4px; "
        "  border: 1px solid #585b70; "
        "  font-weight: bold; "
        "}"
    );

    // Set column widths
    m_tableView->setColumnWidth(ProcessTableModel::PID, 70);
    m_tableView->setColumnWidth(ProcessTableModel::Name, 200);
    m_tableView->setColumnWidth(ProcessTableModel::CPUPercent, 80);
    m_tableView->setColumnWidth(ProcessTableModel::MemoryPercent, 80);
    m_tableView->setColumnWidth(ProcessTableModel::MemoryBytes, 100);

    layout->addWidget(m_tableView, 1);

    // Connect search
    connect(m_searchBox, &QLineEdit::textChanged,
            m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);

    setStyleSheet(
        "QGroupBox { "
        "  color: #cdd6f4; "
        "  border: 1px solid #585b70; "
        "  border-radius: 6px; "
        "  margin-top: 8px; "
        "  padding-top: 14px; "
        "  font-weight: bold; "
        "} "
        "QGroupBox::title { "
        "  subcontrol-origin: margin; "
        "  left: 10px; "
        "  padding: 0 5px; "
        "}"
    );
}

ProcessTableModel* ProcessTable::model() const {
    return m_model;
}

void ProcessTable::updateProcesses(
    const std::vector<ProcessInfo>& processes)
{
    m_model->updateProcesses(processes);
    m_countLabel->setText(QString("%1 processes").arg(processes.size()));
}
