#include "memory_monitor.h"
#include <QHBoxLayout>

// ---- MemoryMonitorChart ----

MemoryMonitorChart::MemoryMonitorChart(QWidget* parent)
    : GLChartWidget(parent)
    , m_usedBuffer(300)
    , m_cachedBuffer(300)
    , m_swapBuffer(300)
{
    connect(this, &GLChartWidget::initialized,
            this, &MemoryMonitorChart::initRenderers);
}

MemoryMonitorChart::~MemoryMonitorChart() = default;

void MemoryMonitorChart::setBufferSize(int size) {
    m_bufferSize = size;
    m_usedBuffer.resize(size);
    m_cachedBuffer.resize(size);
    m_swapBuffer.resize(size);
}

void MemoryMonitorChart::initRenderers() {
    if (m_renderersInitialized || !renderer()) return;

    m_areaChart = std::make_unique<LineGraph>();
    m_areaChart->initialize(renderer());

    LineGraph::Style areaStyle;
    areaStyle.showFill = true;
    areaStyle.autoScaleY = false;
    areaStyle.yMin = 0.0f;
    areaStyle.yMax = 100.0f;
    areaStyle.gridYDivisions = 4;
    m_areaChart->setStyle(areaStyle);

    m_breakdownChart = std::make_unique<BarChart>();
    m_breakdownChart->initialize(renderer());

    BarChart::Style barStyle;
    barStyle.autoScaleY = false;
    barStyle.yMax = 100.0f;
    barStyle.barSpacing = 0.25f;
    barStyle.horizontal = true;
    m_breakdownChart->setStyle(barStyle);

    m_renderersInitialized = true;
}

void MemoryMonitorChart::updateMetrics(const MemoryMetrics& metrics) {
    m_latestMetrics = metrics;

    m_usedBuffer.push(static_cast<float>(metrics.used_percent()));
    m_cachedBuffer.push(static_cast<float>(metrics.cached_percent()));
    m_swapBuffer.push(static_cast<float>(metrics.swap_percent()));

    update();
}

void MemoryMonitorChart::renderChart() {
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

    // Top 60%: filled area chart for memory history
    float areaH = h * 0.55f;
    float barH = h * 0.35f;
    float gap = h * 0.10f;

    // Render area chart with memory usage history
    m_areaChart->clearData();

    auto usedData = m_usedBuffer.snapshot();
    auto cachedData = m_cachedBuffer.snapshot();
    auto swapData = m_swapBuffer.snapshot();

    if (!usedData.empty()) {
        m_areaChart->setData("Used", usedData, QColor(166, 227, 161));
    }
    if (!cachedData.empty()) {
        m_areaChart->setData("Cached", cachedData, QColor(249, 226, 175));
    }
    if (!swapData.empty()) {
        m_areaChart->setData("Swap", swapData, QColor(243, 139, 168));
    }

    m_areaChart->render(x, y + barH + gap, w, areaH);

    // Bottom: horizontal bar chart showing current breakdown
    std::vector<BarChart::Bar> bars;

    BarChart::Bar usedBar;
    usedBar.label = "Used";
    usedBar.value = static_cast<float>(m_latestMetrics.used_percent());
    usedBar.color = QColor(166, 227, 161);
    bars.push_back(usedBar);

    BarChart::Bar cachedBar;
    cachedBar.label = "Cached";
    cachedBar.value = static_cast<float>(m_latestMetrics.cached_percent());
    cachedBar.color = QColor(249, 226, 175);
    bars.push_back(cachedBar);

    BarChart::Bar freeBar;
    freeBar.label = "Free";
    freeBar.value = m_latestMetrics.total_bytes > 0
        ? static_cast<float>(
              static_cast<double>(m_latestMetrics.free_bytes)
              / m_latestMetrics.total_bytes * 100.0)
        : 0.0f;
    freeBar.color = QColor(108, 112, 134);
    bars.push_back(freeBar);

    if (m_latestMetrics.swap_total_bytes > 0) {
        BarChart::Bar swapBar;
        swapBar.label = "Swap";
        swapBar.value = static_cast<float>(m_latestMetrics.swap_percent());
        swapBar.color = QColor(243, 139, 168);
        bars.push_back(swapBar);
    }

    m_breakdownChart->setBars(bars);
    m_breakdownChart->render(x, y, w, barH);
}

// ---- MemoryMonitor (composite widget) ----

MemoryMonitor::MemoryMonitor(QWidget* parent)
    : QGroupBox("Memory Monitor", parent)
{
    auto* layout = new QVBoxLayout(this);

    m_chart = new MemoryMonitorChart(this);
    m_chart->setMinimumHeight(200);
    layout->addWidget(m_chart, 1);

    // RAM progress bar
    auto* ramLayout = new QHBoxLayout();
    ramLayout->addWidget(new QLabel("RAM:"));
    m_ramBar = new QProgressBar();
    m_ramBar->setRange(0, 100);
    m_ramBar->setTextVisible(true);
    m_ramBar->setStyleSheet(
        "QProgressBar { background: #313244; border: 1px solid #585b70; "
        "border-radius: 3px; text-align: center; color: #cdd6f4; } "
        "QProgressBar::chunk { background: #a6e3a1; border-radius: 2px; }"
    );
    ramLayout->addWidget(m_ramBar, 1);
    layout->addLayout(ramLayout);

    // Swap progress bar
    auto* swapLayout = new QHBoxLayout();
    swapLayout->addWidget(new QLabel("Swap:"));
    m_swapBar = new QProgressBar();
    m_swapBar->setRange(0, 100);
    m_swapBar->setTextVisible(true);
    m_swapBar->setStyleSheet(
        "QProgressBar { background: #313244; border: 1px solid #585b70; "
        "border-radius: 3px; text-align: center; color: #cdd6f4; } "
        "QProgressBar::chunk { background: #f38ba8; border-radius: 2px; }"
    );
    swapLayout->addWidget(m_swapBar, 1);
    layout->addLayout(swapLayout);

    // Info labels
    auto* infoLayout = new QHBoxLayout();

    m_usedLabel = new QLabel("Used: --");
    m_usedLabel->setStyleSheet("color: #a6e3a1;");
    infoLayout->addWidget(m_usedLabel);

    m_cachedLabel = new QLabel("Cached: --");
    m_cachedLabel->setStyleSheet("color: #f9e2af;");
    infoLayout->addWidget(m_cachedLabel);

    m_freeLabel = new QLabel("Free: --");
    m_freeLabel->setStyleSheet("color: #6c7086;");
    infoLayout->addWidget(m_freeLabel);

    m_swapLabel = new QLabel("Swap: --");
    m_swapLabel->setStyleSheet("color: #f38ba8;");
    infoLayout->addWidget(m_swapLabel);

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
        "} "
        "QLabel { color: #cdd6f4; }"
    );
}

MemoryMonitorChart* MemoryMonitor::chart() const {
    return m_chart;
}

void MemoryMonitor::setBufferSize(int size) {
    m_chart->setBufferSize(size);
}

void MemoryMonitor::updateMetrics(const MemoryMetrics& metrics) {
    m_chart->updateMetrics(metrics);

    m_usedLabel->setText(QString("Used: %1").arg(formatBytes(metrics.used_bytes)));
    m_freeLabel->setText(QString("Free: %1").arg(formatBytes(metrics.free_bytes)));
    m_cachedLabel->setText(QString("Cached: %1").arg(formatBytes(metrics.cached_bytes)));
    m_swapLabel->setText(QString("Swap: %1 / %2")
                             .arg(formatBytes(metrics.swap_used_bytes))
                             .arg(formatBytes(metrics.swap_total_bytes)));

    m_ramBar->setValue(static_cast<int>(metrics.used_percent()));
    m_ramBar->setFormat(QString("%1% (%2 / %3)")
                            .arg(static_cast<int>(metrics.used_percent()))
                            .arg(formatBytes(metrics.used_bytes))
                            .arg(formatBytes(metrics.total_bytes)));

    m_swapBar->setValue(static_cast<int>(metrics.swap_percent()));
    m_swapBar->setFormat(QString("%1%").arg(static_cast<int>(metrics.swap_percent())));
}

QString MemoryMonitor::formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double value = static_cast<double>(bytes);

    while (value >= 1024.0 && unitIndex < 4) {
        value /= 1024.0;
        ++unitIndex;
    }

    return QString("%1 %2").arg(value, 0, 'f', 1).arg(units[unitIndex]);
}
