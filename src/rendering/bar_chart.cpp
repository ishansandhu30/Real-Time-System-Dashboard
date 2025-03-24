#include "bar_chart.h"
#include <algorithm>
#include <cmath>

BarChart::BarChart() = default;
BarChart::~BarChart() {
    delete m_customShader;
}

void BarChart::initialize(GLRenderer* renderer) {
    m_renderer = renderer;
}

void BarChart::loadShaders(const QString& vertPath, const QString& fragPath) {
    if (!m_renderer) return;
    delete m_customShader;
    m_customShader = m_renderer->loadShaderProgramFromFiles(vertPath, fragPath);
}

void BarChart::setBars(const std::vector<Bar>& bars) {
    m_bars = bars;
    if (m_style.autoScaleY) {
        computeAutoScale();
    }
}

void BarChart::setValues(const std::vector<float>& values,
                          const std::vector<std::string>& labels,
                          const QColor& baseColor)
{
    m_bars.clear();
    m_bars.reserve(values.size());

    for (size_t i = 0; i < values.size(); ++i) {
        Bar bar;
        bar.value = values[i];
        bar.label = (i < labels.size()) ? labels[i] : std::to_string(i);

        // Create color variation based on index
        float hue = baseColor.hueF() + static_cast<float>(i) * 0.05f;
        if (hue > 1.0f) hue -= 1.0f;
        bar.color = QColor::fromHsvF(hue, baseColor.saturationF(),
                                      baseColor.valueF(), baseColor.alphaF());
        m_bars.push_back(std::move(bar));
    }

    if (m_style.autoScaleY) {
        computeAutoScale();
    }
}

void BarChart::setStyle(const Style& style) {
    m_style = style;
    if (m_style.autoScaleY) {
        computeAutoScale();
    }
}

const BarChart::Style& BarChart::style() const {
    return m_style;
}

void BarChart::computeAutoScale() {
    float maxVal = 0.0f;
    for (const auto& bar : m_bars) {
        maxVal = std::max(maxVal, bar.value);
    }

    if (maxVal < 1.0f) maxVal = 100.0f;

    // Round up to a nice number
    float magnitude = std::pow(10.0f, std::floor(std::log10(maxVal)));
    m_computedYMax = std::ceil(maxVal / magnitude) * magnitude;

    // Ensure at least 10% headroom
    if (m_computedYMax < maxVal * 1.1f) {
        m_computedYMax = maxVal * 1.1f;
    }
}

void BarChart::render(float x, float y, float width, float height) {
    if (!m_renderer || m_bars.empty()) return;

    QOpenGLShaderProgram* shader = m_customShader;

    // Draw background
    m_renderer->drawRect(x, y, width, height, m_style.bgColor, shader);

    // Draw grid
    if (m_style.showGrid) {
        int xDivs = static_cast<int>(m_bars.size());
        m_renderer->drawGrid(x, x + width, y, y + height,
                              xDivs, m_style.gridYDivisions,
                              m_style.gridColor);
    }

    float yMax = m_style.autoScaleY ? m_computedYMax : m_style.yMax;
    if (yMax < 0.001f) yMax = 100.0f;

    int numBars = static_cast<int>(m_bars.size());

    if (!m_style.horizontal) {
        // Vertical bars
        float totalBarWidth = width / numBars;
        float gap = totalBarWidth * m_style.barSpacing;
        float barWidth = totalBarWidth - gap;

        for (int i = 0; i < numBars; ++i) {
            float normalizedHeight = (m_bars[i].value / yMax) * height;
            normalizedHeight = std::clamp(normalizedHeight, 0.0f, height);

            float barX = x + i * totalBarWidth + gap * 0.5f;
            float barY = y;

            m_renderer->drawRect(barX, barY, barWidth, normalizedHeight,
                                  m_bars[i].color, shader);

            // Draw bar outline
            std::vector<float> outline = {
                barX, barY,
                barX + barWidth, barY,
                barX + barWidth, barY + normalizedHeight,
                barX, barY + normalizedHeight,
                barX, barY
            };
            QColor outlineColor = m_bars[i].color.darker(130);
            m_renderer->drawLineStrip(outline, outlineColor, 1.0f, shader);
        }
    } else {
        // Horizontal bars
        float totalBarHeight = height / numBars;
        float gap = totalBarHeight * m_style.barSpacing;
        float barHeight = totalBarHeight - gap;

        for (int i = 0; i < numBars; ++i) {
            float normalizedWidth = (m_bars[i].value / yMax) * width;
            normalizedWidth = std::clamp(normalizedWidth, 0.0f, width);

            float barX = x;
            float barY = y + i * totalBarHeight + gap * 0.5f;

            m_renderer->drawRect(barX, barY, normalizedWidth, barHeight,
                                  m_bars[i].color, shader);
        }
    }

    // Draw border
    std::vector<float> border = {
        x, y,
        x + width, y,
        x + width, y + height,
        x, y + height,
        x, y
    };
    m_renderer->drawLineStrip(border, m_style.borderColor, 1.0f, shader);
}
