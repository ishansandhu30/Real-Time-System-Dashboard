#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QColorDialog>
#include <QPushButton>
#include <QJsonObject>

/// Configuration dialog for dashboard settings including polling rate,
/// visible modules, color themes, and buffer sizes.
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    struct Settings {
        int pollingIntervalMs       = 1000;
        int processPollingIntervalMs= 2000;
        int bufferSize              = 300;
        int refreshRateFps          = 60;
        bool showCpuMonitor         = true;
        bool showMemoryMonitor      = true;
        bool showProcessTable       = true;
        bool showPerCore            = true;
        bool showSwap               = true;
        QString theme               = "dark";
        QColor cpuLineColor         = QColor(137, 180, 250);
        QColor memoryUsedColor      = QColor(166, 227, 161);
        QColor swapColor            = QColor(243, 139, 168);
    };

    explicit SettingsDialog(const Settings& current, QWidget* parent = nullptr);

    Settings settings() const;

    /// Load settings from a JSON object (e.g. from dashboard.json).
    static Settings fromJson(const QJsonObject& json);

    /// Convert settings to JSON.
    static QJsonObject toJson(const Settings& settings);

signals:
    void settingsChanged(const SettingsDialog::Settings& newSettings);

private slots:
    void onAccepted();
    void pickColor(QPushButton* button, QColor& targetColor);

private:
    void setupUi();

    QSpinBox* m_pollingInterval;
    QSpinBox* m_processPollingInterval;
    QSpinBox* m_bufferSize;
    QSpinBox* m_refreshRate;
    QCheckBox* m_showCpu;
    QCheckBox* m_showMemory;
    QCheckBox* m_showProcess;
    QCheckBox* m_showPerCore;
    QCheckBox* m_showSwap;
    QComboBox* m_themeCombo;
    QPushButton* m_cpuColorBtn;
    QPushButton* m_memColorBtn;
    QPushButton* m_swapColorBtn;

    Settings m_settings;
};

Q_DECLARE_METATYPE(SettingsDialog::Settings)

#endif // SETTINGS_DIALOG_H
