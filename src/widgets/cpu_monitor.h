#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <vector>
#include <memory>
#include "gl_chart_widget.h"
#include "rendering/line_graph.h"
#include "rendering/bar_chart.h"
#include "data/metrics.h"
#include "data/data_buffer.h"

/// CPU usage monitoring widget displaying real-time per-core line graphs.
/// Uses GLChartWidget for OpenGL rendering. Shows current usage,
/// running average, and peak values with numeric labels.
class CpuMonitorChart : public GLChartWidget {
    Q_OBJECT

public:
    explicit CpuMonitorChart(QWidget* parent = nullptr);
    ~CpuMonitorChart() override;

    void setBufferSize(int size);

public slots:
    void updateMetrics(const CpuMetrics& metrics);

protected:
    void renderChart() override;

private:
    void initRenderers();
    bool m_renderersInitialized = false;

    std::unique_ptr<LineGraph> m_lineGraph;
    std::unique_ptr<BarChart> m_barChart;

    // Per-core history buffers
    std::vector<DataBuffer<float>> m_coreBuffers;
    DataBuffer<float> m_totalBuffer;

    CpuMetrics m_latestMetrics;
    int m_bufferSize = 300;
};

/// Complete CPU Monitor widget with chart and info labels.
class CpuMonitor : public QGroupBox {
    Q_OBJECT

public:
    explicit CpuMonitor(QWidget* parent = nullptr);

    CpuMonitorChart* chart() const;
    void setBufferSize(int size);

public slots:
    void updateMetrics(const CpuMetrics& metrics);

private:
    CpuMonitorChart* m_chart;
    QLabel* m_currentLabel;
    QLabel* m_averageLabel;
    QLabel* m_peakLabel;
    QLabel* m_coreCountLabel;
};

#endif // CPU_MONITOR_H
