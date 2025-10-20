#include "visualization/Renderer.h"

#include <algorithm>
#include <array>
#include <stdexcept>

#include <QOpenGLFunctions>

namespace {
const char *kVertexShader = R"(\
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uViewProj;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)";

const char *kFragmentShader = R"(\
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)";
}

namespace {
GLuint compileShader(QOpenGLFunctions *gl, GLenum type, const char *source) {
    GLuint shader = gl->glCreateShader(type);
    gl->glShaderSource(shader, 1, &source, nullptr);
    gl->glCompileShader(shader);

    GLint status = GL_FALSE;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char log[512] = {0};
        gl->glGetShaderInfoLog(shader, 512, nullptr, log);
        gl->glDeleteShader(shader);
        throw std::runtime_error(std::string("Échec compilation shader: ") + log);
    }
    return shader;
}
}

Renderer::Renderer() = default;

Renderer::~Renderer() {
    if (!m_gl)
        return;
    if (m_program) {
        m_gl->glDeleteProgram(m_program);
        m_program = 0;
    }
    if (m_vbo) {
        m_gl->glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_vao) {
        m_gl->glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void Renderer::initialize(QOpenGLFunctions *functions) {
    m_gl = functions;
    ensureInitialized();
}

void Renderer::setScene(std::shared_ptr<Scene> scene) { m_scene = std::move(scene); }

void Renderer::resize(int w, int h) {
    m_width = w;
    m_height = h;
}

void Renderer::beginFrame(const glm::mat4 &viewProj) {
    ensureInitialized();
    m_viewProj = viewProj;
    m_vertices.clear();

    if (!m_gl)
        return;

    m_gl->glEnable(GL_DEPTH_TEST);
    m_gl->glPolygonMode(GL_FRONT_AND_BACK, m_wireframe ? GL_LINE : GL_FILL);
}

void Renderer::drawAxes(float length) {
    const glm::vec4 xColor(1.0f, 0.0f, 0.0f, 1.0f);
    const glm::vec4 yColor(0.0f, 1.0f, 0.0f, 1.0f);
    const glm::vec4 zColor(0.0f, 0.5f, 1.0f, 1.0f);

    appendLine(glm::vec3(0.0f), glm::vec3(length, 0.0f, 0.0f), xColor, xColor);
    appendLine(glm::vec3(0.0f), glm::vec3(0.0f, length, 0.0f), yColor, yColor);
    appendLine(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, length), zColor, zColor);
}

void Renderer::drawGrid(float size, float step, float fade) {
    if (step <= 0.0f)
        return;

    int halfLines = static_cast<int>(size / step);
    glm::vec4 majorColor(0.35f, 0.35f, 0.4f, 0.6f);
    glm::vec4 minorColor(0.2f, 0.2f, 0.25f, std::clamp(fade, 0.05f, 0.6f));

    for (int i = -halfLines; i <= halfLines; ++i) {
        float offset = static_cast<float>(i) * step;
        glm::vec4 color = (i == 0) ? majorColor : minorColor;

        appendLine(glm::vec3(-size, 0.0f, offset), glm::vec3(size, 0.0f, offset), color, color);
        appendLine(glm::vec3(offset, 0.0f, -size), glm::vec3(offset, 0.0f, size), color, color);
    }
}

void Renderer::drawAABB(const glm::vec3 &minCorner, const glm::vec3 &maxCorner, const glm::vec4 &color) {
    glm::vec3 corners[8] = {
        {minCorner.x, minCorner.y, minCorner.z},
        {maxCorner.x, minCorner.y, minCorner.z},
        {minCorner.x, maxCorner.y, minCorner.z},
        {maxCorner.x, maxCorner.y, minCorner.z},
        {minCorner.x, minCorner.y, maxCorner.z},
        {maxCorner.x, minCorner.y, maxCorner.z},
        {minCorner.x, maxCorner.y, maxCorner.z},
        {maxCorner.x, maxCorner.y, maxCorner.z},
    };

    auto addEdge = [&](int a, int b) {
        appendLine(corners[a], corners[b], color, color);
    };

    addEdge(0, 1); addEdge(1, 3); addEdge(3, 2); addEdge(2, 0);
    addEdge(4, 5); addEdge(5, 7); addEdge(7, 6); addEdge(6, 4);
    addEdge(0, 4); addEdge(1, 5); addEdge(2, 6); addEdge(3, 7);
}

void Renderer::drawCross(const glm::vec3 &position, float size, const glm::vec4 &color) {
    float half = size * 0.5f;
    appendLine(position - glm::vec3(half, 0.0f, 0.0f), position + glm::vec3(half, 0.0f, 0.0f), color, color);
    appendLine(position - glm::vec3(0.0f, half, 0.0f), position + glm::vec3(0.0f, half, 0.0f), color, color);
    appendLine(position - glm::vec3(0.0f, 0.0f, half), position + glm::vec3(0.0f, 0.0f, half), color, color);
}

void Renderer::endFrame() {
    if (!m_gl || m_vertices.empty())
        return;

    ensureInitialized();

    m_gl->glBindVertexArray(m_vao);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    m_gl->glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
                       m_vertices.data(), GL_DYNAMIC_DRAW);

    m_gl->glUseProgram(m_program);
    GLint loc = m_gl->glGetUniformLocation(m_program, "uViewProj");
    if (loc >= 0) {
        m_gl->glUniformMatrix4fv(loc, 1, GL_FALSE, matrixPtr(m_viewProj));
    }

    m_gl->glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_vertices.size()));
    m_gl->glBindVertexArray(0);

    m_vertices.clear();
    m_gl->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::setWireframeEnabled(bool enabled) { m_wireframe = enabled; }

void Renderer::ensureInitialized() {
    if (!m_gl || m_program)
        return;

    GLuint vertex = compileShader(m_gl, GL_VERTEX_SHADER, kVertexShader);
    GLuint fragment = compileShader(m_gl, GL_FRAGMENT_SHADER, kFragmentShader);

    m_program = m_gl->glCreateProgram();
    m_gl->glAttachShader(m_program, vertex);
    m_gl->glAttachShader(m_program, fragment);
    m_gl->glLinkProgram(m_program);

    GLint linked = GL_FALSE;
    m_gl->glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    m_gl->glDeleteShader(vertex);
    m_gl->glDeleteShader(fragment);

    if (linked != GL_TRUE) {
        char log[512] = {0};
        m_gl->glGetProgramInfoLog(m_program, 512, nullptr, log);
        m_gl->glDeleteProgram(m_program);
        m_program = 0;
        throw std::runtime_error(std::string("Échec linkage programme: ") + log);
    }

    m_gl->glGenVertexArrays(1, &m_vao);
    m_gl->glGenBuffers(1, &m_vbo);

    m_gl->glBindVertexArray(m_vao);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    m_gl->glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(0));
    m_gl->glEnableVertexAttribArray(1);
    m_gl->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(sizeof(glm::vec3)));
    m_gl->glBindVertexArray(0);
}

void Renderer::appendLine(const glm::vec3 &a, const glm::vec3 &b, const glm::vec4 &colorA, const glm::vec4 &colorB) {
    m_vertices.push_back(Vertex{a, colorA});
    m_vertices.push_back(Vertex{b, colorB});
}

const float *Renderer::matrixPtr(const glm::mat4 &m) const {
#ifdef USE_BASIC_MATH
    return m.m;
#else
    return glm::value_ptr(m);
#endif
}
