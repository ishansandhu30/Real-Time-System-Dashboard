#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QColor>
#include <QMatrix4x4>
#include <QString>
#include <memory>
#include <vector>

/// Core OpenGL rendering utility class.
/// Manages shader programs, VAOs/VBOs, and provides drawing primitives
/// for lines, filled areas, grids, and text rendering.
class GLRenderer : public QOpenGLFunctions_3_3_Core {
public:
    GLRenderer();
    ~GLRenderer();

    /// Initialize OpenGL functions and create shared resources.
    bool initialize();

    /// Load and compile a shader program from source strings.
    QOpenGLShaderProgram* loadShaderProgram(
        const QString& vertexSource,
        const QString& fragmentSource);

    /// Load shaders from file paths.
    QOpenGLShaderProgram* loadShaderProgramFromFiles(
        const QString& vertexPath,
        const QString& fragmentPath);

    /// Set the projection matrix (typically orthographic for 2D charts).
    void setProjectionMatrix(const QMatrix4x4& projection);
    const QMatrix4x4& projectionMatrix() const;

    /// Set the model-view matrix.
    void setModelViewMatrix(const QMatrix4x4& modelView);
    const QMatrix4x4& modelViewMatrix() const;

    // ---- Drawing primitives ----

    /// Draw a line strip from a set of 2D points.
    void drawLineStrip(const std::vector<float>& vertices,
                       const QColor& color,
                       float lineWidth = 2.0f,
                       QOpenGLShaderProgram* shader = nullptr);

    /// Draw a filled area (triangle strip) between two Y boundaries.
    void drawFilledArea(const std::vector<float>& vertices,
                        const QColor& color,
                        QOpenGLShaderProgram* shader = nullptr);

    /// Draw a filled rectangle.
    void drawRect(float x, float y, float width, float height,
                  const QColor& color,
                  QOpenGLShaderProgram* shader = nullptr);

    /// Draw grid lines across the viewport.
    void drawGrid(float xMin, float xMax, float yMin, float yMax,
                  int xDivisions, int yDivisions,
                  const QColor& color, float lineWidth = 1.0f);

    /// Cleanup all GPU resources.
    void cleanup();

private:
    void ensureVAO();

    QMatrix4x4 m_projection;
    QMatrix4x4 m_modelView;

    // Shared VAO/VBO for dynamic geometry
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;
    bool m_initialized;

    // Default shader for simple colored geometry
    std::unique_ptr<QOpenGLShaderProgram> m_defaultShader;

    void createDefaultShader();
};

#endif // GL_RENDERER_H
