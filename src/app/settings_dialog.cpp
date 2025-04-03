#include "settings_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTabWidget>

SettingsDialog::SettingsDialog(const Settings& current, QWidget* parent)
    : QDialog(parent)
    , m_settings(current)
{
    setWindowTitle("Dashboard Settings");
    setMinimumWidth(450);
    setupUi();
}

void SettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* tabs = new QTabWidget(this);

    // ---- General Tab ----
    auto* generalTab = new QWidget();
    auto* generalLayout = new QFormLayout(generalTab);

    m_pollingInterval = new QSpinBox();
    m_pollingInterval->setRange(100, 10000);
    m_pollingInterval->setSuffix(" ms");
    m_pollingInterval->setValue(m_settings.pollingIntervalMs);
    m_pollingInterval->setToolTip("How frequently to poll CPU and memory metrics");
    generalLayout->addRow("Polling Interval:", m_pollingInterval);

    m_processPollingInterval = new QSpinBox();
    m_processPollingInterval->setRange(500, 30000);
    m_processPollingInterval->setSuffix(" ms");
    m_processPollingInterval->setValue(m_settings.processPollingIntervalMs);
    m_processPollingInterval->setToolTip("How frequently to update the process list");
    generalLayout->addRow("Process Polling:", m_processPollingInterval);

    m_bufferSize = new QSpinBox();
    m_bufferSize->setRange(50, 10000);
    m_bufferSize->setSuffix(" samples");
    m_bufferSize->setValue(m_settings.bufferSize);
    m_bufferSize->setToolTip("Number of historical samples kept in memory");
    generalLayout->addRow("Buffer Size:", m_bufferSize);

    m_refreshRate = new QSpinBox();
    m_refreshRate->setRange(10, 144);
    m_refreshRate->setSuffix(" fps");
    m_refreshRate->setValue(m_settings.refreshRateFps);
    m_refreshRate->setToolTip("Target UI refresh rate");
    generalLayout->addRow("Refresh Rate:", m_refreshRate);

    tabs->addTab(generalTab, "General");

    // ---- Modules Tab ----
    auto* modulesTab = new QWidget();
    auto* modulesLayout = new QVBoxLayout(modulesTab);

    m_showCpu = new QCheckBox("CPU Monitor");
    m_showCpu->setChecked(m_settings.showCpuMonitor);
    modulesLayout->addWidget(m_showCpu);

    m_showPerCore = new QCheckBox("  Show per-core details");
    m_showPerCore->setChecked(m_settings.showPerCore);
    modulesLayout->addWidget(m_showPerCore);

    m_showMemory = new QCheckBox("Memory Monitor");
    m_showMemory->setChecked(m_settings.showMemoryMonitor);
    modulesLayout->addWidget(m_showMemory);

    m_showSwap = new QCheckBox("  Show swap usage");
    m_showSwap->setChecked(m_settings.showSwap);
    modulesLayout->addWidget(m_showSwap);

    m_showProcess = new QCheckBox("Process Table");
    m_showProcess->setChecked(m_settings.showProcessTable);
    modulesLayout->addWidget(m_showProcess);

    modulesLayout->addStretch();
    tabs->addTab(modulesTab, "Modules");

    // ---- Appearance Tab ----
    auto* appearanceTab = new QWidget();
    auto* appearanceLayout = new QFormLayout(appearanceTab);

    m_themeCombo = new QComboBox();
    m_themeCombo->addItems({"dark", "light"});
    m_themeCombo->setCurrentText(m_settings.theme);
    appearanceLayout->addRow("Theme:", m_themeCombo);

    auto createColorButton = [](const QColor& color) -> QPushButton* {
        auto* btn = new QPushButton();
        btn->setFixedSize(60, 24);
        btn->setStyleSheet(QString("background-color: %1; border: 1px solid #585b70; "
                                    "border-radius: 3px;").arg(color.name()));
        return btn;
    };

    m_cpuColorBtn = createColorButton(m_settings.cpuLineColor);
    connect(m_cpuColorBtn, &QPushButton::clicked, this, [this]() {
        pickColor(m_cpuColorBtn, m_settings.cpuLineColor);
    });
    appearanceLayout->addRow("CPU Line Color:", m_cpuColorBtn);

    m_memColorBtn = createColorButton(m_settings.memoryUsedColor);
    connect(m_memColorBtn, &QPushButton::clicked, this, [this]() {
        pickColor(m_memColorBtn, m_settings.memoryUsedColor);
    });
    appearanceLayout->addRow("Memory Color:", m_memColorBtn);

    m_swapColorBtn = createColorButton(m_settings.swapColor);
    connect(m_swapColorBtn, &QPushButton::clicked, this, [this]() {
        pickColor(m_swapColorBtn, m_settings.swapColor);
    });
    appearanceLayout->addRow("Swap Color:", m_swapColorBtn);

    tabs->addTab(appearanceTab, "Appearance");

    mainLayout->addWidget(tabs);

    // Buttons
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    // Style the dialog
    setStyleSheet(
        "QDialog { background: #1e1e2e; color: #cdd6f4; } "
        "QTabWidget::pane { border: 1px solid #585b70; background: #1e1e2e; } "
        "QTabBar::tab { background: #313244; color: #cdd6f4; padding: 6px 12px; "
        "  border: 1px solid #585b70; border-bottom: none; border-radius: 4px 4px 0 0; } "
        "QTabBar::tab:selected { background: #45475a; } "
        "QGroupBox { color: #cdd6f4; border: 1px solid #585b70; border-radius: 4px; "
        "  margin-top: 6px; padding-top: 10px; } "
        "QSpinBox, QComboBox { background: #313244; color: #cdd6f4; "
        "  border: 1px solid #585b70; border-radius: 3px; padding: 2px 6px; } "
        "QCheckBox { color: #cdd6f4; } "
        "QCheckBox::indicator { width: 16px; height: 16px; } "
        "QLabel { color: #cdd6f4; } "
        "QPushButton { background: #313244; color: #cdd6f4; border: 1px solid #585b70; "
        "  border-radius: 4px; padding: 4px 12px; } "
        "QPushButton:hover { background: #45475a; }"
    );
}

void SettingsDialog::onAccepted() {
    m_settings.pollingIntervalMs = m_pollingInterval->value();
    m_settings.processPollingIntervalMs = m_processPollingInterval->value();
    m_settings.bufferSize = m_bufferSize->value();
    m_settings.refreshRateFps = m_refreshRate->value();
    m_settings.showCpuMonitor = m_showCpu->isChecked();
    m_settings.showMemoryMonitor = m_showMemory->isChecked();
    m_settings.showProcessTable = m_showProcess->isChecked();
    m_settings.showPerCore = m_showPerCore->isChecked();
    m_settings.showSwap = m_showSwap->isChecked();
    m_settings.theme = m_themeCombo->currentText();

    emit settingsChanged(m_settings);
    accept();
}

void SettingsDialog::pickColor(QPushButton* button, QColor& targetColor) {
    QColor color = QColorDialog::getColor(targetColor, this, "Select Color");
    if (color.isValid()) {
        targetColor = color;
        button->setStyleSheet(
            QString("background-color: %1; border: 1px solid #585b70; "
                    "border-radius: 3px;").arg(color.name()));
    }
}

SettingsDialog::Settings SettingsDialog::settings() const {
    return m_settings;
}

SettingsDialog::Settings SettingsDialog::fromJson(const QJsonObject& json) {
    Settings s;

    auto dc = json["data_collection"].toObject();
    s.pollingIntervalMs = dc["polling_interval_ms"].toInt(1000);
    s.processPollingIntervalMs = dc["process_polling_interval_ms"].toInt(2000);
    s.bufferSize = dc["buffer_size"].toInt(300);

    auto general = json["general"].toObject();
    s.refreshRateFps = general["refresh_rate_fps"].toInt(60);

    auto modules = json["modules"].toObject();
    s.showCpuMonitor = modules["cpu_monitor"].toObject()["enabled"].toBool(true);
    s.showPerCore = modules["cpu_monitor"].toObject()["show_per_core"].toBool(true);
    s.showMemoryMonitor = modules["memory_monitor"].toObject()["enabled"].toBool(true);
    s.showSwap = modules["memory_monitor"].toObject()["show_swap"].toBool(true);
    s.showProcessTable = modules["process_table"].toObject()["enabled"].toBool(true);

    auto appearance = json["appearance"].toObject();
    s.theme = appearance["theme"].toString("dark");

    auto colors = appearance["colors"].toObject();
    s.cpuLineColor = QColor(colors["cpu_line"].toString("#89b4fa"));
    s.memoryUsedColor = QColor(colors["memory_used"].toString("#a6e3a1"));
    s.swapColor = QColor(colors["swap_used"].toString("#f38ba8"));

    return s;
}

QJsonObject SettingsDialog::toJson(const Settings& s) {
    QJsonObject json;

    QJsonObject general;
    general["refresh_rate_fps"] = s.refreshRateFps;
    json["general"] = general;

    QJsonObject dc;
    dc["polling_interval_ms"] = s.pollingIntervalMs;
    dc["process_polling_interval_ms"] = s.processPollingIntervalMs;
    dc["buffer_size"] = s.bufferSize;
    json["data_collection"] = dc;

    QJsonObject cpuMod;
    cpuMod["enabled"] = s.showCpuMonitor;
    cpuMod["show_per_core"] = s.showPerCore;

    QJsonObject memMod;
    memMod["enabled"] = s.showMemoryMonitor;
    memMod["show_swap"] = s.showSwap;

    QJsonObject procMod;
    procMod["enabled"] = s.showProcessTable;

    QJsonObject modules;
    modules["cpu_monitor"] = cpuMod;
    modules["memory_monitor"] = memMod;
    modules["process_table"] = procMod;
    json["modules"] = modules;

    QJsonObject colors;
    colors["cpu_line"] = s.cpuLineColor.name();
    colors["memory_used"] = s.memoryUsedColor.name();
    colors["swap_used"] = s.swapColor.name();

    QJsonObject appearance;
    appearance["theme"] = s.theme;
    appearance["colors"] = colors;
    json["appearance"] = appearance;

    return json;
}
