#include "line_graph.h"
#include <algorithm>
#include <cmath>

LineGraph::LineGraph() = default;
LineGraph::~LineGraph() {
    delete m_customShader;
}

void LineGraph::initialize(GLRenderer* renderer) {
    m_renderer = renderer;
}

void LineGraph::loadShaders(const QString& vertPath, const QString& fragPath) {
    if (!m_renderer) return;
    delete m_customShader;
    m_customShader = m_renderer->loadShaderProgramFromFiles(vertPath, fragPath);
}

void LineGraph::setData(const std::vector<float>& values) {
    m_series.clear();
    Series s;
    s.name = "default";
    s.values = values;
    s.color = m_style.lineColor;
    m_series.push_back(std::move(s));

    if (m_style.autoScaleY) {
        computeAutoScale();
    }
}

void LineGraph::setData(const std::string& seriesName,
                         const std::vector<float>& values,
                         const QColor& color)
{
    // Update existing series or add new one
    for (auto& s : m_series) {
        if (s.name == seriesName) {
            s.values = values;
            s.color = color;
            if (m_style.autoScaleY) computeAutoScale();
            return;
        }
    }

    Series s;
    s.name = seriesName;
    s.values = values;
    s.color = color;
    m_series.push_back(std::move(s));

    if (m_style.autoScaleY) {
        computeAutoScale();
    }
}

void LineGraph::clearData() {
    m_series.clear();
}

void LineGraph::setStyle(const Style& style) {
    m_style = style;
    if (m_style.autoScaleY) {
        computeAutoScale();
    }
}

const LineGraph::Style& LineGraph::style() const {
    return m_style;
}

void LineGraph::computeAutoScale() {
    float globalMin = std::numeric_limits<float>::max();
    float globalMax = std::numeric_limits<float>::lowest();

    for (const auto& series : m_series) {
        for (float v : series.values) {
            globalMin = std::min(globalMin, v);
            globalMax = std::max(globalMax, v);
        }
    }

    if (globalMin == std::numeric_limits<float>::max()) {
        m_computedYMin = 0.0f;
        m_computedYMax = 100.0f;
        return;
    }

    // Add 10% padding
    float range = globalMax - globalMin;
    if (range < 1.0f) range = 1.0f;
    m_computedYMin = std::max(0.0f, globalMin - range * 0.05f);
    m_computedYMax = globalMax + range * 0.1f;

    // Round to nice numbers
    float step = range / m_style.gridYDivisions;
    float magnitude = std::pow(10.0f, std::floor(std::log10(step)));
    step = std::ceil(step / magnitude) * magnitude;

    m_computedYMin = std::floor(m_computedYMin / step) * step;
    m_computedYMax = std::ceil(m_computedYMax / step) * step;
}

std::vector<float> LineGraph::buildLineVertices(const std::vector<float>& values,
                                                  float x, float y,
                                                  float width, float height)
{
    std::vector<float> verts;
    if (values.empty()) return verts;

    float yMin = m_style.autoScaleY ? m_computedYMin : m_style.yMin;
    float yMax = m_style.autoScaleY ? m_computedYMax : m_style.yMax;
    float yRange = yMax - yMin;
    if (yRange < 0.001f) yRange = 1.0f;

    verts.reserve(values.size() * 2);

    for (size_t i = 0; i < values.size(); ++i) {
        float xNorm = values.size() > 1
            ? static_cast<float>(i) / (values.size() - 1)
            : 0.5f;
        float yNorm = (values[i] - yMin) / yRange;
        yNorm = std::clamp(yNorm, 0.0f, 1.0f);

        verts.push_back(x + xNorm * width);
        verts.push_back(y + yNorm * height);
    }

    return verts;
}

std::vector<float> LineGraph::buildFillVertices(const std::vector<float>& values,
                                                  float x, float y,
                                                  float width, float height)
{
    std::vector<float> verts;
    if (values.empty()) return verts;

    float yMin = m_style.autoScaleY ? m_computedYMin : m_style.yMin;
    float yMax = m_style.autoScaleY ? m_computedYMax : m_style.yMax;
    float yRange = yMax - yMin;
    if (yRange < 0.001f) yRange = 1.0f;

    // Build triangle strip: alternating bottom-top pairs
    verts.reserve(values.size() * 4);

    for (size_t i = 0; i < values.size(); ++i) {
        float xNorm = values.size() > 1
            ? static_cast<float>(i) / (values.size() - 1)
            : 0.5f;
        float yNorm = (values[i] - yMin) / yRange;
        yNorm = std::clamp(yNorm, 0.0f, 1.0f);

        float px = x + xNorm * width;

        // Bottom vertex (baseline)
        verts.push_back(px);
        verts.push_back(y);

        // Top vertex (data point)
        verts.push_back(px);
        verts.push_back(y + yNorm * height);
    }

    return verts;
}

void LineGraph::render(float x, float y, float width, float height) {
    if (!m_renderer || m_series.empty()) return;

    QOpenGLShaderProgram* shader = m_customShader;

    // Draw background
    m_renderer->drawRect(x, y, width, height, m_style.bgColor, shader);

    // Draw grid
    if (m_style.showGrid) {
        m_renderer->drawGrid(x, x + width, y, y + height,
                              m_style.gridXDivisions, m_style.gridYDivisions,
                              m_style.gridColor);
    }

    // Draw each series
    for (const auto& series : m_series) {
        // Draw fill area if enabled
        if (m_style.showFill) {
            auto fillVerts = buildFillVertices(series.values, x, y, width, height);
            QColor fillColor = series.color;
            fillColor.setAlpha(64);
            m_renderer->drawFilledArea(fillVerts, fillColor, shader);
        }

        // Draw the line
        auto lineVerts = buildLineVertices(series.values, x, y, width, height);
        m_renderer->drawLineStrip(lineVerts, series.color,
                                   m_style.lineWidth, shader);
    }

    // Draw border
    std::vector<float> border = {
        x, y,
        x + width, y,
        x + width, y + height,
        x, y + height,
        x, y
    };
    m_renderer->drawLineStrip(border, m_style.axisColor, 1.0f, shader);
}
