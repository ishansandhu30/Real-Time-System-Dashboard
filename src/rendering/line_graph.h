#ifndef LINE_GRAPH_H
#define LINE_GRAPH_H

#include "gl_renderer.h"
#include <QColor>
#include <QString>
#include <vector>
#include <memory>

/// OpenGL line graph renderer with configurable colors, auto-scaling Y axis,
/// grid lines, and smooth anti-aliased line rendering.
class LineGraph {
public:
    struct Style {
        QColor lineColor    = QColor(137, 180, 250);   // #89b4fa
        QColor fillColor    = QColor(137, 180, 250, 64);
        QColor gridColor    = QColor(58, 58, 74);      // #3a3a4a
        QColor axisColor    = QColor(205, 214, 244);    // #cdd6f4
        QColor bgColor      = QColor(30, 30, 46);       // #1e1e2e
        float lineWidth     = 2.0f;
        bool showFill       = false;
        bool showGrid       = true;
        bool autoScaleY     = true;
        float yMin          = 0.0f;
        float yMax          = 100.0f;
        int gridXDivisions  = 10;
        int gridYDivisions  = 5;
        QString xLabel;
        QString yLabel;
    };

    LineGraph();
    ~LineGraph();

    /// Initialize with a reference to a GLRenderer (must be already initialized).
    void initialize(GLRenderer* renderer);

    /// Load custom shaders (optional - uses renderer defaults otherwise).
    void loadShaders(const QString& vertPath, const QString& fragPath);

    /// Set the data points to render. Each value is a Y value; X is evenly spaced.
    void setData(const std::vector<float>& values);

    /// Set a named data series (for multi-line support).
    void setData(const std::string& seriesName,
                 const std::vector<float>& values,
                 const QColor& color);

    /// Clear all data series.
    void clearData();

    /// Set the style configuration.
    void setStyle(const Style& style);
    const Style& style() const;

    /// Render the line graph into the current OpenGL context.
    /// chartRect defines the drawing area in pixel coordinates: (x, y, width, height).
    void render(float x, float y, float width, float height);

private:
    struct Series {
        std::string name;
        std::vector<float> values;
        QColor color;
    };

    void computeAutoScale();
    std::vector<float> buildLineVertices(const std::vector<float>& values,
                                          float x, float y,
                                          float width, float height);
    std::vector<float> buildFillVertices(const std::vector<float>& values,
                                          float x, float y,
                                          float width, float height);

    GLRenderer* m_renderer = nullptr;
    QOpenGLShaderProgram* m_customShader = nullptr;
    Style m_style;
    std::vector<Series> m_series;
    float m_computedYMin = 0.0f;
    float m_computedYMax = 100.0f;
};

#endif // LINE_GRAPH_H
