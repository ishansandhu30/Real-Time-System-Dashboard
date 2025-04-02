#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
#include <QGridLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QAction>
#include <memory>
#include "settings_dialog.h"
#include "widgets/cpu_monitor.h"
#include "widgets/memory_monitor.h"
#include "widgets/process_table.h"
#include "data/system_data_collector.h"

/// Main dashboard window hosting all monitoring widgets in a grid layout.
/// Provides a menu bar with File (export, quit), View (toggle modules),
/// and Settings. Uses QTimer for periodic UI refresh targeting 60fps.
class DashboardWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit DashboardWindow(QWidget* parent = nullptr);
    ~DashboardWindow() override;

    /// Load configuration from a JSON file.
    void loadConfig(const QString& configPath);

private slots:
    void onExportData();
    void onOpenSettings();
    void onSettingsChanged(const SettingsDialog::Settings& newSettings);
    void onToggleCpuMonitor(bool visible);
    void onToggleMemoryMonitor(bool visible);
    void onToggleProcessTable(bool visible);
    void onRefreshTick();

private:
    void setupMenuBar();
    void setupWidgets();
    void setupDataCollector();
    void applySettings(const SettingsDialog::Settings& settings);
    void updateStatusBar();

    // Layout
    QWidget* m_centralWidget;
    QGridLayout* m_gridLayout;

    // Widgets
    CpuMonitor* m_cpuMonitor;
    MemoryMonitor* m_memoryMonitor;
    ProcessTable* m_processTable;

    // Data collection
    std::unique_ptr<SystemDataCollector> m_dataCollector;

    // Refresh timer
    QTimer* m_refreshTimer;
    int m_frameCount = 0;
    double m_fps = 0.0;
    QElapsedTimer m_fpsTimer;

    // Menu actions
    QAction* m_toggleCpuAction;
    QAction* m_toggleMemoryAction;
    QAction* m_toggleProcessAction;

    // Current settings
    SettingsDialog::Settings m_settings;
};

#endif // DASHBOARD_H
