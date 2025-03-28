#include "cpu_monitor.h"
#include <QHBoxLayout>
#include <algorithm>
#include <numeric>

// ---- CpuMonitorChart ----

CpuMonitorChart::CpuMonitorChart(QWidget* parent)
    : GLChartWidget(parent)
    , m_totalBuffer(300)
{
    connect(this, &GLChartWidget::initialized, this, &CpuMonitorChart::initRenderers);
}

CpuMonitorChart::~CpuMonitorChart() = default;

void CpuMonitorChart::setBufferSize(int size) {
    m_bufferSize = size;
    m_totalBuffer.resize(size);
    for (auto& buf : m_coreBuffers) {
        buf.resize(size);
    }
}

void CpuMonitorChart::initRenderers() {
    if (m_renderersInitialized || !renderer()) return;

    m_lineGraph = std::make_unique<LineGraph>();
    m_lineGraph->initialize(renderer());

    LineGraph::Style style;
    style.lineColor = QColor(137, 180, 250);
    style.fillColor = QColor(137, 180, 250, 64);
    style.showFill = true;
    style.autoScaleY = false;
    style.yMin = 0.0f;
    style.yMax = 100.0f;
    m_lineGraph->setStyle(style);

    m_barChart = std::make_unique<BarChart>();
    m_barChart->initialize(renderer());

    BarChart::Style barStyle;
    barStyle.autoScaleY = false;
    barStyle.yMax = 100.0f;
    barStyle.barSpacing = 0.15f;
    m_barChart->setStyle(barStyle);

    m_renderersInitialized = true;
}

void CpuMonitorChart::updateMetrics(const CpuMetrics& metrics) {
    m_latestMetrics = metrics;

    // Ensure we have the right number of core buffers
    while (m_coreBuffers.size() < metrics.per_core.size()) {
        m_coreBuffers.emplace_back(m_bufferSize);
    }

    // Push per-core data
    for (size_t i = 0; i < metrics.per_core.size(); ++i) {
        m_coreBuffers[i].push(static_cast<float>(metrics.per_core[i]));
    }

    // Push total
    m_totalBuffer.push(static_cast<float>(metrics.total_usage));

    update();  // Trigger repaint
}

void CpuMonitorChart::renderChart() {
    if (!m_renderersInitialized) {
        initRenderers();
        if (!m_renderersInitialized) return;
    }

    QRectF area = chartArea();
    float x = static_cast<float>(area.x());
    float y = static_cast<float>(area.y());
    float w = static_cast<float>(area.width());
    float h = static_cast<float>(area.height());

    if (w <= 0 || h <= 0) return;

    // Split: top 70% for line graph, bottom 30% for bar chart
    float lineH = h * 0.65f;
    float barH = h * 0.30f;
    float gap = h * 0.05f;

    // Render line graph with total CPU history
    m_lineGraph->clearData();

    // Add total CPU as main line
    auto totalData = m_totalBuffer.snapshot();
    if (!totalData.empty()) {
        m_lineGraph->setData("Total", totalData, QColor(137, 180, 250));
    }

    // Add per-core lines with different colors
    static const QColor coreColors[] = {
        QColor(166, 227, 161),  // green
        QColor(249, 226, 175),  // yellow
        QColor(243, 139, 168),  // red
        QColor(203, 166, 247),  // mauve
        QColor(137, 220, 235),  // teal
        QColor(250, 179, 135),  // peach
        QColor(148, 226, 213),  // sapphire
        QColor(245, 194, 231),  // pink
    };

    for (size_t i = 0; i < m_coreBuffers.size() && i < 8; ++i) {
        auto coreData = m_coreBuffers[i].snapshot();
        if (!coreData.empty()) {
            std::string name = "Core " + std::to_string(i);
            m_lineGraph->setData(name, coreData,
                                  coreColors[i % 8]);
        }
    }

    m_lineGraph->render(x, y + barH + gap, w, lineH);

    // Render bar chart with current per-core usage
    if (!m_latestMetrics.per_core.empty()) {
        std::vector<BarChart::Bar> bars;
        for (size_t i = 0; i < m_latestMetrics.per_core.size(); ++i) {
            BarChart::Bar bar;
            bar.label = "C" + std::to_string(i);
            bar.value = static_cast<float>(m_latestMetrics.per_core[i]);
            bar.color = coreColors[i % 8];
            bars.push_back(bar);
        }
        m_barChart->setBars(bars);
        m_barChart->render(x, y, w, barH);
    }
}

// ---- CpuMonitor (composite widget) ----

CpuMonitor::CpuMonitor(QWidget* parent)
    : QGroupBox("CPU Monitor", parent)
{
    auto* layout = new QVBoxLayout(this);

    m_chart = new CpuMonitorChart(this);
    m_chart->setMinimumHeight(200);
    layout->addWidget(m_chart, 1);

    // Info labels row
    auto* infoLayout = new QHBoxLayout();

    m_currentLabel = new QLabel("Current: 0.0%");
    m_currentLabel->setStyleSheet("color: #89b4fa; font-weight: bold;");
    infoLayout->addWidget(m_currentLabel);

    m_averageLabel = new QLabel("Average: 0.0%");
    m_averageLabel->setStyleSheet("color: #a6e3a1;");
    infoLayout->addWidget(m_averageLabel);

    m_peakLabel = new QLabel("Peak: 0.0%");
    m_peakLabel->setStyleSheet("color: #f38ba8;");
    infoLayout->addWidget(m_peakLabel);

    m_coreCountLabel = new QLabel("Cores: 0");
    m_coreCountLabel->setStyleSheet("color: #cdd6f4;");
    infoLayout->addWidget(m_coreCountLabel);

    layout->addLayout(infoLayout);

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

CpuMonitorChart* CpuMonitor::chart() const {
    return m_chart;
}

void CpuMonitor::setBufferSize(int size) {
    m_chart->setBufferSize(size);
}

void CpuMonitor::updateMetrics(const CpuMetrics& metrics) {
    m_chart->updateMetrics(metrics);

    m_currentLabel->setText(QString("Current: %1%")
                                .arg(metrics.total_usage, 0, 'f', 1));
    m_averageLabel->setText(QString("Average: %1%")
                                .arg(metrics.average, 0, 'f', 1));
    m_peakLabel->setText(QString("Peak: %1%")
                             .arg(metrics.peak, 0, 'f', 1));
    m_coreCountLabel->setText(QString("Cores: %1")
                                  .arg(metrics.core_count));
}
