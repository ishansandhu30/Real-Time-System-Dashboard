#ifndef GL_CHART_WIDGET_H
#define GL_CHART_WIDGET_H

#include <QOpenGLWidget>
#include <QMatrix4x4>
#include <memory>
#include "rendering/gl_renderer.h"

/// QOpenGLWidget subclass providing the base OpenGL chart functionality.
/// Initializes the OpenGL 3.3 Core context, handles resize events, and
/// sets up orthographic projection matrices for 2D chart rendering.
///
/// Subclasses or composing widgets can access the shared GLRenderer
/// instance to draw charts within this widget's OpenGL context.
class GLChartWidget : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit GLChartWidget(QWidget* parent = nullptr);
    ~GLChartWidget() override;

    /// Access the shared renderer (valid after initializeGL).
    GLRenderer* renderer() const;

    /// Set the background clear color.
    void setBackgroundColor(const QColor& color);

    /// Get chart drawing area in widget-local coordinates.
    QRectF chartArea() const;

    /// Margins around the chart area for labels/axes.
    void setMargins(float left, float top, float right, float bottom);

signals:
    void initialized();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    /// Override in subclasses to perform custom rendering.
    virtual void renderChart();

    std::unique_ptr<GLRenderer> m_renderer;
    QColor m_bgColor;

    float m_marginLeft   = 50.0f;
    float m_marginTop    = 20.0f;
    float m_marginRight  = 20.0f;
    float m_marginBottom = 30.0f;
};

#endif // GL_CHART_WIDGET_H
