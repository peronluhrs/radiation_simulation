#include "visualization/Renderer.h"
#include "visualization/VtkLegacyLoader.h"

#include "common.h"
#include "core/Scene.h"
#include "glm_simple.h"

#include <QMatrix4x4>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QVector4D>
#include <QString>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace {
#ifdef USE_BASIC_MATH
inline void copyToColumnMajor(const glm::mat4 &m, float out[16]) {
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[col * 4 + row] = m.m[row * 4 + col];
        }
    }
}
#else
inline void copyToColumnMajor(const glm::mat4 &m, float out[16]) {
    const float *src = glm::value_ptr(m);
    std::copy(src, src + 16, out);
}
#endif
}

Renderer::Renderer() = default;

Renderer::~Renderer() { destroyBuffers(); }

void Renderer::setViewport(QWidget *w) { m_viewport = w; }

void Renderer::initialize(QWidget *w) {
    setViewport(w);
    ensureInitialized();
}

void Renderer::initialize(QWindow * /*win*/) {
    ensureInitialized();
}

void Renderer::attachScene(std::shared_ptr<Scene> scene) { m_scene = std::move(scene); }

void Renderer::resize(int w, int h) {
    ensureInitialized();
    if (!m_gl)
        return;
    m_gl->glViewport(0, 0, w, h);
}

void Renderer::beginFrame(const glm::mat4 &viewProj) {
    ensureInitialized();
    if (!m_gl)
        return;

    m_viewProj = viewProj;
    m_gl->glEnable(GL_DEPTH_TEST);
    m_gl->glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    m_gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    ensureInitialized();
    if (!m_gl)
        return;

    drawImportedMesh();
}

void Renderer::renderOnce() {
    if (m_viewport)
        m_viewport->update();
}

bool Renderer::loadVtk(const std::string &path, std::string *err) {
    VtkMesh mesh;
    std::string loaderError;
    if (!VtkLegacyLoader::load(path, mesh, &loaderError)) {
        if (err)
            *err = loaderError;
        std::ostringstream oss;
        oss << "Renderer: échec du chargement VTK '" << path << "'";
        if (!loaderError.empty())
            oss << " (" << loaderError << ")";
        Log::error(oss.str());
        return false;
    }

    if (mesh.vertices.empty()) {
        const std::string message = "Le fichier VTK ne contient aucun point.";
        if (err)
            *err = message;
        Log::error("Renderer: " + message);
        return false;
    }

    m_vertices = std::move(mesh.vertices);
    m_triangleIndices = std::move(mesh.triangles);
    m_lineIndices = std::move(mesh.lines);
    m_meshBounds = mesh.bounds;
    ensureMeshEdges();

    m_meshStats.vertexCount = m_vertices.size();
    m_meshStats.triangleCount = m_triangleIndices.size() / 3;
    m_meshStats.lineCount = m_lineIndices.size() / 2;

    if (m_triangleIndices.empty() && m_lineIndices.empty()) {
        const std::string message = "Le fichier VTK ne contient ni triangles ni lignes exploitables.";
        if (err)
            *err = message;
        Log::error("Renderer: " + message);
        return false;
    }

    m_meshLoaded = true;
    m_buffersDirty = true;
    if (m_gl)
        uploadMeshToGPU();

    std::ostringstream oss;
    oss << "VTK import: " << path << " (verts=" << m_meshStats.vertexCount
        << ", tris=" << m_meshStats.triangleCount << ", lines=" << m_meshStats.lineCount << ")";
    Log::info(oss.str());
    return true;
}

void Renderer::clearMesh() {
    destroyBuffers();
    m_vertices.clear();
    m_triangleIndices.clear();
    m_lineIndices.clear();
    m_meshBounds = AABB();
    m_meshStats = MeshStats{};
    m_meshLoaded = false;
    m_buffersDirty = false;
    m_triangleCount = 0;
    m_lineCount = 0;
}

void Renderer::ensureInitialized() {
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return;

    if (!m_gl)
        m_gl = ctx->functions();
    if (!m_gl)
        return;

    if (!m_program)
        createShaders();

    if (ctx)
        m_isGLES = ctx->isOpenGLES();

    if (m_buffersDirty && m_meshLoaded)
        uploadMeshToGPU();
}

void Renderer::createShaders() {
    if (m_program)
        return;

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx)
        return;

    const bool isGles = ctx->isOpenGLES();
    QString lastErrorLog;

    auto compile = [&](const QByteArray &vs, const QByteArray &fs) -> bool {
        auto program = std::make_unique<QOpenGLShaderProgram>();
        if (!program->addShaderFromSourceCode(QOpenGLShader::Vertex, vs)) {
            lastErrorLog = program->log();
            return false;
        }
        if (!program->addShaderFromSourceCode(QOpenGLShader::Fragment, fs)) {
            lastErrorLog = program->log();
            return false;
        }
        program->bindAttributeLocation("a_position", 0);
        if (!program->link()) {
            lastErrorLog = program->log();
            return false;
        }
        m_mvpLocation = program->uniformLocation("u_mvp");
        m_colorLocation = program->uniformLocation("u_color");
        m_program = std::move(program);
        lastErrorLog.clear();
        m_isGLES = isGles;
        return true;
    };

    const QByteArray vs330 = QByteArrayLiteral(
        "#version 330 core\n"
        "layout(location = 0) in vec3 a_position;\n"
        "uniform mat4 u_mvp;\n"
        "void main() {\n"
        "    gl_Position = u_mvp * vec4(a_position, 1.0);\n"
        "}\n");
    const QByteArray fs330 = QByteArrayLiteral(
        "#version 330 core\n"
        "uniform vec4 u_color;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = u_color;\n"
        "}\n");

    const QByteArray vs300 = QByteArrayLiteral(
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(location = 0) in vec3 a_position;\n"
        "uniform mat4 u_mvp;\n"
        "void main() {\n"
        "    gl_Position = u_mvp * vec4(a_position, 1.0);\n"
        "}\n");
    const QByteArray fs300 = QByteArrayLiteral(
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform vec4 u_color;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = u_color;\n"
        "}\n");

    const QByteArray vs120 = QByteArrayLiteral(
        "#version 120\n"
        "attribute vec3 a_position;\n"
        "uniform mat4 u_mvp;\n"
        "void main() {\n"
        "    gl_Position = u_mvp * vec4(a_position, 1.0);\n"
        "}\n");
    const QByteArray fs120 = QByteArrayLiteral(
        "#version 120\n"
        "uniform vec4 u_color;\n"
        "void main() {\n"
        "    gl_FragColor = u_color;\n"
        "}\n");

    const QByteArray vs100 = QByteArrayLiteral(
        "#version 100\n"
        "precision mediump float;\n"
        "attribute vec3 a_position;\n"
        "uniform mat4 u_mvp;\n"
        "void main() {\n"
        "    gl_Position = u_mvp * vec4(a_position, 1.0);\n"
        "}\n");
    const QByteArray fs100 = QByteArrayLiteral(
        "#version 100\n"
        "precision mediump float;\n"
        "uniform vec4 u_color;\n"
        "void main() {\n"
        "    gl_FragColor = u_color;\n"
        "}\n");

    bool ok = false;
    if (isGles) {
        ok = compile(vs300, fs300) || compile(vs100, fs100);
    } else {
        ok = compile(vs330, fs330) || compile(vs120, fs120);
    }

    if (!ok) {
        std::string message = "Renderer: impossible de compiler les shaders OpenGL";
        if (!lastErrorLog.isEmpty())
            message += ": " + lastErrorLog.toStdString();
        else if (m_program)
            message += ": " + m_program->log().toStdString();
        Log::error(message);
        m_program.reset();
    }
}

void Renderer::destroyBuffers() {
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QOpenGLFunctions *gl = m_gl;
    if (!gl && ctx)
        gl = ctx->functions();
    if (!gl)
        return;

    if (m_vertexBuffer) {
        gl->glDeleteBuffers(1, &m_vertexBuffer);
        m_vertexBuffer = 0;
    }
    if (m_triangleBuffer) {
        gl->glDeleteBuffers(1, &m_triangleBuffer);
        m_triangleBuffer = 0;
    }
    if (m_lineBuffer) {
        gl->glDeleteBuffers(1, &m_lineBuffer);
        m_lineBuffer = 0;
    }
    m_triangleCount = 0;
    m_lineCount = 0;
}

void Renderer::uploadMeshToGPU() {
    if (!m_gl || !m_meshLoaded)
        return;

    if (!m_vertexBuffer)
        m_gl->glGenBuffers(1, &m_vertexBuffer);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    m_gl->glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices.size() * sizeof(glm::vec3)), m_vertices.data(), GL_STATIC_DRAW);

    if (!m_triangleIndices.empty()) {
        if (!m_triangleBuffer)
            m_gl->glGenBuffers(1, &m_triangleBuffer);
        m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triangleBuffer);
        m_gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_triangleIndices.size() * sizeof(uint32_t)), m_triangleIndices.data(), GL_STATIC_DRAW);
        m_triangleCount = static_cast<GLsizei>(m_triangleIndices.size());
    } else if (m_triangleBuffer) {
        m_gl->glDeleteBuffers(1, &m_triangleBuffer);
        m_triangleBuffer = 0;
        m_triangleCount = 0;
    }

    if (!m_lineIndices.empty()) {
        if (!m_lineBuffer)
            m_gl->glGenBuffers(1, &m_lineBuffer);
        m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineBuffer);
        m_gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_lineIndices.size() * sizeof(uint32_t)), m_lineIndices.data(), GL_STATIC_DRAW);
        m_lineCount = static_cast<GLsizei>(m_lineIndices.size());
    } else if (m_lineBuffer) {
        m_gl->glDeleteBuffers(1, &m_lineBuffer);
        m_lineBuffer = 0;
        m_lineCount = 0;
    }

    m_gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    m_buffersDirty = false;
}

void Renderer::drawImportedMesh() {
    if (!m_meshLoaded || !m_gl || m_vertices.empty())
        return;

    if (!m_program)
        return;

    if (m_buffersDirty)
        uploadMeshToGPU();

    if (!m_vertexBuffer)
        return;

    float matrixData[16];
    copyToColumnMajor(m_viewProj, matrixData);
    QMatrix4x4 mvp(matrixData);

    m_program->bind();
    m_program->setUniformValue(m_mvpLocation, mvp);

    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void *>(0));

    if (!m_wireframe && m_triangleCount > 0 && m_triangleBuffer) {
        m_program->setUniformValue(m_colorLocation, QVector4D(0.72f, 0.76f, 0.82f, 1.0f));
        m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triangleBuffer);
        m_gl->glDrawElements(GL_TRIANGLES, m_triangleCount, GL_UNSIGNED_INT, nullptr);
    }

    if (m_lineCount > 0 && m_lineBuffer) {
        m_program->setUniformValue(m_colorLocation, QVector4D(0.12f, 0.14f, 0.18f, 1.0f));
        m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineBuffer);
        m_gl->glDrawElements(GL_LINES, m_lineCount, GL_UNSIGNED_INT, nullptr);
    }

    m_gl->glDisableVertexAttribArray(0);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    m_program->release();
}

void Renderer::ensureMeshEdges() {
    if (m_triangleIndices.empty())
        return;

    const size_t originalSize = m_lineIndices.size();
    m_lineIndices.reserve(originalSize + m_triangleIndices.size() * 2);
    for (size_t i = 0; i + 2 < m_triangleIndices.size(); i += 3) {
        uint32_t a = m_triangleIndices[i];
        uint32_t b = m_triangleIndices[i + 1];
        uint32_t c = m_triangleIndices[i + 2];
        m_lineIndices.push_back(a);
        m_lineIndices.push_back(b);
        m_lineIndices.push_back(b);
        m_lineIndices.push_back(c);
        m_lineIndices.push_back(c);
        m_lineIndices.push_back(a);
    }
}
