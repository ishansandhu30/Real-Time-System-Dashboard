#include "gl_chart_widget.h"
#include <QSurfaceFormat>
#include <QDebug>

GLChartWidget::GLChartWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_bgColor(30, 30, 46)  // Dark background
{
    // Request OpenGL 3.3 Core profile
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);  // 4x MSAA for anti-aliasing
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    setFormat(format);

    // Minimum size
    setMinimumSize(200, 150);
}

GLChartWidget::~GLChartWidget() {
    makeCurrent();
    if (m_renderer) {
        m_renderer->cleanup();
    }
    doneCurrent();
}

GLRenderer* GLChartWidget::renderer() const {
    return m_renderer.get();
}

void GLChartWidget::setBackgroundColor(const QColor& color) {
    m_bgColor = color;
    update();
}

QRectF GLChartWidget::chartArea() const {
    return QRectF(
        m_marginLeft,
        m_marginTop,
        width() - m_marginLeft - m_marginRight,
        height() - m_marginTop - m_marginBottom
    );
}

void GLChartWidget::setMargins(float left, float top, float right, float bottom) {
    m_marginLeft = left;
    m_marginTop = top;
    m_marginRight = right;
    m_marginBottom = bottom;
    update();
}

void GLChartWidget::initializeGL() {
    m_renderer = std::make_unique<GLRenderer>();
    if (!m_renderer->initialize()) {
        qWarning() << "GLChartWidget: Failed to initialize GLRenderer";
        return;
    }

    // Enable multisampling
    m_renderer->glEnable(GL_MULTISAMPLE);

    emit initialized();
}

void GLChartWidget::resizeGL(int w, int h) {
    if (!m_renderer) return;

    m_renderer->glViewport(0, 0, w, h);

    // Set up orthographic projection matching widget pixel coordinates.
    // Origin at bottom-left, Y pointing up.
    QMatrix4x4 projection;
    projection.ortho(0.0f, static_cast<float>(w),
                     0.0f, static_cast<float>(h),
                     -1.0f, 1.0f);
    m_renderer->setProjectionMatrix(projection);

    QMatrix4x4 identity;
    identity.setToIdentity();
    m_renderer->setModelViewMatrix(identity);
}

void GLChartWidget::paintGL() {
    if (!m_renderer) return;

    // Clear with background color
    m_renderer->glClearColor(
        m_bgColor.redF(),
        m_bgColor.greenF(),
        m_bgColor.blueF(),
        m_bgColor.alphaF()
    );
    m_renderer->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderChart();
}

void GLChartWidget::renderChart() {
    // Default implementation does nothing.
    // Subclasses or composing widgets override this to draw their charts.
}
