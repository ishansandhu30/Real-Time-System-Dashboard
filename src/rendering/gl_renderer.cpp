#include "gl_renderer.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

GLRenderer::GLRenderer()
    : m_vbo(QOpenGLBuffer::VertexBuffer)
    , m_initialized(false)
{
    m_modelView.setToIdentity();
    m_projection.setToIdentity();
}

GLRenderer::~GLRenderer() {
    cleanup();
}

bool GLRenderer::initialize() {
    if (m_initialized) return true;

    if (!initializeOpenGLFunctions()) {
        qWarning() << "GLRenderer: Failed to initialize OpenGL 3.3 Core functions";
        return false;
    }

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable line smoothing
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    createDefaultShader();
    ensureVAO();

    m_initialized = true;
    return true;
}

void GLRenderer::createDefaultShader() {
    const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec2 position;
        uniform mat4 u_projection;
        uniform mat4 u_modelView;
        void main() {
            gl_Position = u_projection * u_modelView * vec4(position, 0.0, 1.0);
        }
    )";

    const char* fragSrc = R"(
        #version 330 core
        uniform vec4 u_color;
        out vec4 fragColor;
        void main() {
            fragColor = u_color;
        }
    )";

    m_defaultShader = std::make_unique<QOpenGLShaderProgram>();
    m_defaultShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc);
    m_defaultShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc);
    if (!m_defaultShader->link()) {
        qWarning() << "GLRenderer: Default shader link failed:"
                    << m_defaultShader->log();
    }
}

void GLRenderer::ensureVAO() {
    if (!m_vao.isCreated()) {
        m_vao.create();
    }
    if (!m_vbo.isCreated()) {
        m_vbo.create();
        m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    }
}

QOpenGLShaderProgram* GLRenderer::loadShaderProgram(
    const QString& vertexSource,
    const QString& fragmentSource)
{
    auto* program = new QOpenGLShaderProgram();
    if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        qWarning() << "Vertex shader compilation failed:" << program->log();
        delete program;
        return nullptr;
    }
    if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        qWarning() << "Fragment shader compilation failed:" << program->log();
        delete program;
        return nullptr;
    }
    if (!program->link()) {
        qWarning() << "Shader program link failed:" << program->log();
        delete program;
        return nullptr;
    }
    return program;
}

QOpenGLShaderProgram* GLRenderer::loadShaderProgramFromFiles(
    const QString& vertexPath,
    const QString& fragmentPath)
{
    auto readFile = [](const QString& path) -> QString {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Cannot open shader file:" << path;
            return {};
        }
        QTextStream stream(&file);
        return stream.readAll();
    };

    QString vertSrc = readFile(vertexPath);
    QString fragSrc = readFile(fragmentPath);

    if (vertSrc.isEmpty() || fragSrc.isEmpty()) {
        return nullptr;
    }

    return loadShaderProgram(vertSrc, fragSrc);
}

void GLRenderer::setProjectionMatrix(const QMatrix4x4& projection) {
    m_projection = projection;
}

const QMatrix4x4& GLRenderer::projectionMatrix() const {
    return m_projection;
}

void GLRenderer::setModelViewMatrix(const QMatrix4x4& modelView) {
    m_modelView = modelView;
}

const QMatrix4x4& GLRenderer::modelViewMatrix() const {
    return m_modelView;
}

void GLRenderer::drawLineStrip(const std::vector<float>& vertices,
                                const QColor& color,
                                float lineWidth,
                                QOpenGLShaderProgram* shader)
{
    if (vertices.size() < 4) return;  // Need at least 2 points (x,y each)

    QOpenGLShaderProgram* prog = shader ? shader : m_defaultShader.get();
    if (!prog) return;

    prog->bind();
    prog->setUniformValue("u_projection", m_projection);
    prog->setUniformValue("u_modelView", m_modelView);
    prog->setUniformValue("u_color", color);

    m_vao.bind();
    m_vbo.bind();
    m_vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glLineWidth(lineWidth);
    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 2));

    glDisableVertexAttribArray(0);
    m_vbo.release();
    m_vao.release();
    prog->release();
}

void GLRenderer::drawFilledArea(const std::vector<float>& vertices,
                                 const QColor& color,
                                 QOpenGLShaderProgram* shader)
{
    if (vertices.size() < 6) return;  // Need at least 3 vertices

    QOpenGLShaderProgram* prog = shader ? shader : m_defaultShader.get();
    if (!prog) return;

    prog->bind();
    prog->setUniformValue("u_projection", m_projection);
    prog->setUniformValue("u_modelView", m_modelView);
    prog->setUniformValue("u_color", color);

    m_vao.bind();
    m_vbo.bind();
    m_vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 2));

    glDisableVertexAttribArray(0);
    m_vbo.release();
    m_vao.release();
    prog->release();
}

void GLRenderer::drawRect(float x, float y, float width, float height,
                           const QColor& color,
                           QOpenGLShaderProgram* shader)
{
    // Two triangles forming a rectangle (triangle strip)
    std::vector<float> verts = {
        x,         y,
        x + width, y,
        x,         y + height,
        x + width, y + height
    };
    drawFilledArea(verts, color, shader);
}

void GLRenderer::drawGrid(float xMin, float xMax, float yMin, float yMax,
                           int xDivisions, int yDivisions,
                           const QColor& color, float lineWidth)
{
    QOpenGLShaderProgram* prog = m_defaultShader.get();
    if (!prog) return;

    std::vector<float> gridVerts;

    // Vertical lines
    for (int i = 0; i <= xDivisions; ++i) {
        float x = xMin + (xMax - xMin) * i / xDivisions;
        gridVerts.push_back(x); gridVerts.push_back(yMin);
        gridVerts.push_back(x); gridVerts.push_back(yMax);
    }

    // Horizontal lines
    for (int i = 0; i <= yDivisions; ++i) {
        float y = yMin + (yMax - yMin) * i / yDivisions;
        gridVerts.push_back(xMin); gridVerts.push_back(y);
        gridVerts.push_back(xMax); gridVerts.push_back(y);
    }

    prog->bind();
    prog->setUniformValue("u_projection", m_projection);
    prog->setUniformValue("u_modelView", m_modelView);
    prog->setUniformValue("u_color", color);

    m_vao.bind();
    m_vbo.bind();
    m_vbo.allocate(gridVerts.data(),
                   static_cast<int>(gridVerts.size() * sizeof(float)));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glLineWidth(lineWidth);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(gridVerts.size() / 2));

    glDisableVertexAttribArray(0);
    m_vbo.release();
    m_vao.release();
    prog->release();
}

void GLRenderer::cleanup() {
    if (!m_initialized) return;

    m_defaultShader.reset();
    if (m_vbo.isCreated()) m_vbo.destroy();
    if (m_vao.isCreated()) m_vao.destroy();

    m_initialized = false;
}
