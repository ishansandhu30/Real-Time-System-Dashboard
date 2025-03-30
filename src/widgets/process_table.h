#ifndef PROCESS_TABLE_H
#define PROCESS_TABLE_H

#include <QWidget>
#include <QTableView>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <vector>
#include <mutex>
#include "data/metrics.h"

/// Custom table model for displaying running processes.
/// Thread-safe: data can be updated from the collector thread.
class ProcessTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        PID = 0,
        Name,
        CPUPercent,
        MemoryPercent,
        MemoryBytes,
        Status,
        ColumnCount
    };

    explicit ProcessTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

public slots:
    /// Update the process list. Thread-safe.
    void updateProcesses(const std::vector<ProcessInfo>& processes);

private:
    static QString formatBytes(uint64_t bytes);

    std::vector<ProcessInfo> m_processes;
    mutable std::mutex m_mutex;
};

/// Complete Process Table widget with search, sorting, and count display.
class ProcessTable : public QGroupBox {
    Q_OBJECT

public:
    explicit ProcessTable(QWidget* parent = nullptr);

    ProcessTableModel* model() const;

public slots:
    void updateProcesses(const std::vector<ProcessInfo>& processes);

private:
    ProcessTableModel* m_model;
    QSortFilterProxyModel* m_proxyModel;
    QTableView* m_tableView;
    QLineEdit* m_searchBox;
    QLabel* m_countLabel;
};

#endif // PROCESS_TABLE_H
