#pragma once

#include <memory>
#include <vector>

#include "glm_simple.h"

#include <QOpenGLFunctions>

class Scene;

class Renderer {
  public:
    Renderer();
    ~Renderer();

    void initialize(QOpenGLFunctions *functions);
    void setScene(std::shared_ptr<Scene> scene);
    void resize(int w, int h);

    void beginFrame(const glm::mat4 &viewProj);
    void drawAxes(float length = 1.5f);
    void drawGrid(float size = 10.0f, float step = 1.0f, float fade = 0.25f);
    void drawAABB(const glm::vec3 &minCorner, const glm::vec3 &maxCorner, const glm::vec4 &color);
    void drawCross(const glm::vec3 &position, float size, const glm::vec4 &color);
    void endFrame();

    void setWireframeEnabled(bool enabled);
    bool wireframeEnabled() const { return m_wireframe; }

  private:
    struct Vertex {
        glm::vec3 position;
        glm::vec4 color;
    };

    void ensureInitialized();
    void appendLine(const glm::vec3 &a, const glm::vec3 &b, const glm::vec4 &colorA, const glm::vec4 &colorB);
    const float *matrixPtr(const glm::mat4 &m) const;

    QOpenGLFunctions *m_gl = nullptr;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_program = 0;
    std::vector<Vertex> m_vertices;
    glm::mat4 m_viewProj{1.0f};

    std::shared_ptr<Scene> m_scene;
    bool m_wireframe = false;
    int m_width = 0;
    int m_height = 0;
};
