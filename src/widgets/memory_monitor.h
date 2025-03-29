#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QProgressBar>
#include <memory>
#include "gl_chart_widget.h"
#include "rendering/line_graph.h"
#include "rendering/bar_chart.h"
#include "data/metrics.h"
#include "data/data_buffer.h"

/// Memory monitoring OpenGL chart showing RAM and swap usage as filled
/// area charts, with used/free/cached breakdown.
class MemoryMonitorChart : public GLChartWidget {
    Q_OBJECT

public:
    explicit MemoryMonitorChart(QWidget* parent = nullptr);
    ~MemoryMonitorChart() override;

    void setBufferSize(int size);

public slots:
    void updateMetrics(const MemoryMetrics& metrics);

protected:
    void renderChart() override;

private:
    void initRenderers();
    bool m_renderersInitialized = false;

    std::unique_ptr<LineGraph> m_areaChart;
    std::unique_ptr<BarChart> m_breakdownChart;

    DataBuffer<float> m_usedBuffer;
    DataBuffer<float> m_cachedBuffer;
    DataBuffer<float> m_swapBuffer;

    MemoryMetrics m_latestMetrics;
    int m_bufferSize = 300;
};

/// Complete Memory Monitor widget with chart and info labels.
class MemoryMonitor : public QGroupBox {
    Q_OBJECT

public:
    explicit MemoryMonitor(QWidget* parent = nullptr);

    MemoryMonitorChart* chart() const;
    void setBufferSize(int size);

public slots:
    void updateMetrics(const MemoryMetrics& metrics);

private:
    static QString formatBytes(uint64_t bytes);

    MemoryMonitorChart* m_chart;
    QLabel* m_usedLabel;
    QLabel* m_freeLabel;
    QLabel* m_cachedLabel;
    QLabel* m_swapLabel;
    QProgressBar* m_ramBar;
    QProgressBar* m_swapBar;
};

#endif // MEMORY_MONITOR_H
