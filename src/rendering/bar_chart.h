#ifndef BAR_CHART_H
#define BAR_CHART_H

#include "gl_renderer.h"
#include <QColor>
#include <QString>
#include <vector>
#include <string>

/// OpenGL bar chart renderer for comparative views such as per-core CPU
/// usage or memory breakdown categories.
class BarChart {
public:
    struct Bar {
        std::string label;
        float value;
        QColor color;
    };

    struct Style {
        QColor bgColor      = QColor(30, 30, 46);
        QColor gridColor    = QColor(58, 58, 74);
        QColor axisColor    = QColor(205, 214, 244);
        QColor borderColor  = QColor(88, 91, 112);
        float barSpacing    = 0.2f;   // Fraction of bar width used as gap
        bool showGrid       = true;
        bool autoScaleY     = true;
        float yMin          = 0.0f;
        float yMax          = 100.0f;
        int gridYDivisions  = 5;
        bool horizontal     = false;  // If true, bars are horizontal
    };

    BarChart();
    ~BarChart();

    /// Initialize with a reference to a GLRenderer.
    void initialize(GLRenderer* renderer);

    /// Load custom shaders (optional).
    void loadShaders(const QString& vertPath, const QString& fragPath);

    /// Set the bars to display.
    void setBars(const std::vector<Bar>& bars);

    /// Convenience: set values with a default color gradient.
    void setValues(const std::vector<float>& values,
                   const std::vector<std::string>& labels,
                   const QColor& baseColor = QColor(137, 180, 250));

    /// Set the style configuration.
    void setStyle(const Style& style);
    const Style& style() const;

    /// Render the bar chart into the current OpenGL context.
    void render(float x, float y, float width, float height);

private:
    void computeAutoScale();

    GLRenderer* m_renderer = nullptr;
    QOpenGLShaderProgram* m_customShader = nullptr;
    Style m_style;
    std::vector<Bar> m_bars;
    float m_computedYMax = 100.0f;
};

#endif // BAR_CHART_H
