#include "dashboard.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QDateTime>
#include <QElapsedTimer>
#include <QDebug>

DashboardWindow::DashboardWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Real-Time System Dashboard");
    resize(1400, 900);

    // Apply dark theme stylesheet
    setStyleSheet(
        "QMainWindow { background: #1e1e2e; } "
        "QMenuBar { background: #181825; color: #cdd6f4; border-bottom: 1px solid #313244; } "
        "QMenuBar::item:selected { background: #45475a; } "
        "QMenu { background: #1e1e2e; color: #cdd6f4; border: 1px solid #585b70; } "
        "QMenu::item:selected { background: #45475a; } "
        "QStatusBar { background: #181825; color: #a6adc8; border-top: 1px solid #313244; } "
    );

    setupMenuBar();
    setupWidgets();
    setupDataCollector();

    // Refresh timer for UI updates targeting 60fps
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &DashboardWindow::onRefreshTick);

    m_fpsTimer.start();

    // Default settings
    m_settings.refreshRateFps = 60;
    m_refreshTimer->start(1000 / m_settings.refreshRateFps);

    statusBar()->showMessage("Dashboard ready");
}

DashboardWindow::~DashboardWindow() {
    if (m_dataCollector) {
        m_dataCollector->stop();
        m_dataCollector->wait(3000);
    }
}

void DashboardWindow::loadConfig(const QString& configPath) {
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open config file:" << configPath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in config file:" << configPath;
        return;
    }

    m_settings = SettingsDialog::fromJson(doc.object());
    applySettings(m_settings);

    // Apply window settings from config
    QJsonObject general = doc.object()["general"].toObject();
    if (general.contains("window_width") && general.contains("window_height")) {
        resize(general["window_width"].toInt(1400),
               general["window_height"].toInt(900));
    }
    if (general.contains("window_title")) {
        setWindowTitle(general["window_title"].toString());
    }

    qDebug() << "Loaded config from" << configPath;
}

void DashboardWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();

    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");

    QAction* exportAction = fileMenu->addAction("&Export Data...");
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAction, &QAction::triggered, this, &DashboardWindow::onExportData);

    fileMenu->addSeparator();

    QAction* quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    // View menu
    QMenu* viewMenu = menuBar->addMenu("&View");

    m_toggleCpuAction = viewMenu->addAction("CPU Monitor");
    m_toggleCpuAction->setCheckable(true);
    m_toggleCpuAction->setChecked(true);
    connect(m_toggleCpuAction, &QAction::toggled,
            this, &DashboardWindow::onToggleCpuMonitor);

    m_toggleMemoryAction = viewMenu->addAction("Memory Monitor");
    m_toggleMemoryAction->setCheckable(true);
    m_toggleMemoryAction->setChecked(true);
    connect(m_toggleMemoryAction, &QAction::toggled,
            this, &DashboardWindow::onToggleMemoryMonitor);

    m_toggleProcessAction = viewMenu->addAction("Process Table");
    m_toggleProcessAction->setCheckable(true);
    m_toggleProcessAction->setChecked(true);
    connect(m_toggleProcessAction, &QAction::toggled,
            this, &DashboardWindow::onToggleProcessTable);

    // Settings menu
    QMenu* settingsMenu = menuBar->addMenu("&Settings");
    QAction* settingsAction = settingsMenu->addAction("&Configure...");
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered,
            this, &DashboardWindow::onOpenSettings);
}

void DashboardWindow::setupWidgets() {
    m_centralWidget = new QWidget(this);
    m_gridLayout = new QGridLayout(m_centralWidget);
    m_gridLayout->setSpacing(8);
    m_gridLayout->setContentsMargins(8, 8, 8, 8);

    // CPU Monitor: top-left
    m_cpuMonitor = new CpuMonitor(m_centralWidget);
    m_gridLayout->addWidget(m_cpuMonitor, 0, 0);

    // Memory Monitor: top-right
    m_memoryMonitor = new MemoryMonitor(m_centralWidget);
    m_gridLayout->addWidget(m_memoryMonitor, 0, 1);

    // Process Table: bottom, spanning full width
    m_processTable = new ProcessTable(m_centralWidget);
    m_gridLayout->addWidget(m_processTable, 1, 0, 1, 2);

    // Row stretch: charts get 60%, process table 40%
    m_gridLayout->setRowStretch(0, 6);
    m_gridLayout->setRowStretch(1, 4);

    // Equal column stretch
    m_gridLayout->setColumnStretch(0, 1);
    m_gridLayout->setColumnStretch(1, 1);

    setCentralWidget(m_centralWidget);
}

void DashboardWindow::setupDataCollector() {
    m_dataCollector = std::make_unique<SystemDataCollector>();

    // Connect data signals to widgets
    connect(m_dataCollector.get(), &SystemDataCollector::cpuMetricsUpdated,
            m_cpuMonitor, &CpuMonitor::updateMetrics,
            Qt::QueuedConnection);

    connect(m_dataCollector.get(), &SystemDataCollector::memoryMetricsUpdated,
            m_memoryMonitor, &MemoryMonitor::updateMetrics,
            Qt::QueuedConnection);

    connect(m_dataCollector.get(), &SystemDataCollector::processListUpdated,
            m_processTable, &ProcessTable::updateProcesses,
            Qt::QueuedConnection);

    m_dataCollector->start();
}

void DashboardWindow::applySettings(const SettingsDialog::Settings& settings) {
    // Update data collector
    if (m_dataCollector) {
        m_dataCollector->setPollingInterval(settings.pollingIntervalMs);
        m_dataCollector->setProcessPollingInterval(settings.processPollingIntervalMs);
    }

    // Update buffer sizes
    m_cpuMonitor->setBufferSize(settings.bufferSize);
    m_memoryMonitor->setBufferSize(settings.bufferSize);

    // Update module visibility
    m_cpuMonitor->setVisible(settings.showCpuMonitor);
    m_memoryMonitor->setVisible(settings.showMemoryMonitor);
    m_processTable->setVisible(settings.showProcessTable);

    m_toggleCpuAction->setChecked(settings.showCpuMonitor);
    m_toggleMemoryAction->setChecked(settings.showMemoryMonitor);
    m_toggleProcessAction->setChecked(settings.showProcessTable);

    // Update refresh timer
    if (settings.refreshRateFps > 0) {
        m_refreshTimer->setInterval(1000 / settings.refreshRateFps);
    }
}

void DashboardWindow::onExportData() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Metrics Data", "system_metrics.csv",
        "CSV Files (*.csv);;All Files (*)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Error",
                             "Could not open file for writing: " + fileName);
        return;
    }

    QTextStream out(&file);
    out << "timestamp,metric,value\n";
    out << QDateTime::currentDateTime().toString(Qt::ISODate)
        << ",export_complete,1\n";

    statusBar()->showMessage("Data exported to " + fileName, 5000);
}

void DashboardWindow::onOpenSettings() {
    auto* dialog = new SettingsDialog(m_settings, this);
    connect(dialog, &SettingsDialog::settingsChanged,
            this, &DashboardWindow::onSettingsChanged);
    dialog->exec();
    dialog->deleteLater();
}

void DashboardWindow::onSettingsChanged(
    const SettingsDialog::Settings& newSettings)
{
    m_settings = newSettings;
    applySettings(m_settings);
    statusBar()->showMessage("Settings applied", 3000);
}

void DashboardWindow::onToggleCpuMonitor(bool visible) {
    m_cpuMonitor->setVisible(visible);
    m_settings.showCpuMonitor = visible;
}

void DashboardWindow::onToggleMemoryMonitor(bool visible) {
    m_memoryMonitor->setVisible(visible);
    m_settings.showMemoryMonitor = visible;
}

void DashboardWindow::onToggleProcessTable(bool visible) {
    m_processTable->setVisible(visible);
    m_settings.showProcessTable = visible;
}

void DashboardWindow::onRefreshTick() {
    ++m_frameCount;

    // Calculate FPS every second
    qint64 elapsed = m_fpsTimer.elapsed();
    if (elapsed >= 1000) {
        m_fps = m_frameCount * 1000.0 / elapsed;
        m_frameCount = 0;
        m_fpsTimer.restart();
        updateStatusBar();
    }

    // Trigger OpenGL widget repaints
    if (m_cpuMonitor->isVisible()) {
        m_cpuMonitor->chart()->update();
    }
    if (m_memoryMonitor->isVisible()) {
        m_memoryMonitor->chart()->update();
    }
}

void DashboardWindow::updateStatusBar() {
    statusBar()->showMessage(
        QString("FPS: %1 | Polling: %2ms | Buffer: %3 samples")
            .arg(m_fps, 0, 'f', 1)
            .arg(m_settings.pollingIntervalMs)
            .arg(m_settings.bufferSize)
    );
}
