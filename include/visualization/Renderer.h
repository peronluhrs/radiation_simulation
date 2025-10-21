#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <QWidget>
#include <QWindow>
#include <QOpenGLFunctions>
#include <QOpenGLContext>

#include "geometry/Object3D.h"

class QOpenGLShaderProgram;
struct VtkMesh;

class Scene;

/**
 * Renderer minimal et compatible GLES :
 * - Utilise uniquement QOpenGLFunctions (pas de VAO, pas de glPolygonMode).
 * - Fournit les méthodes attendues par View3D (stubs no-op pour le moment).
 */
class Renderer {
  public:
    Renderer();
    ~Renderer();

    // API basique
    void setViewport(QWidget *w);
    void attachScene(std::shared_ptr<Scene> scene);
    void resize(int w, int h);
    void beginFrame(const glm::mat4 &viewProj);
    void endFrame();
    void renderOnce();

    // Méthodes attendues par View3D (alias / stubs)
    void initialize(QWidget *w);   // setViewport + ensureInitialized (QOpenGLWidget)
    void initialize(QWindow *win); // ensureInitialized (QOpenGLWindow)
    void setScene(std::shared_ptr<Scene> s) { attachScene(std::move(s)); }
    void setWireframeEnabled(bool on) { setWireframe(on); }

    // Configuration “wireframe” (no-op pour l’instant sous GLES)
    void setWireframe(bool on) { m_wireframe = on; }
    bool wireframe() const { return m_wireframe; }

    struct MeshStats {
        size_t vertexCount = 0;
        size_t triangleCount = 0;
        size_t lineCount = 0;
    };

    bool loadVtk(const std::string &path, std::string *err = nullptr);
    bool hasMesh() const { return m_meshLoaded; }
    const AABB &meshBounds() const { return m_meshBounds; }
    MeshStats meshStats() const { return m_meshStats; }

    void clearMesh();

    // Stubs de dessin (pour compiler ; implémentations à faire plus tard)
    void drawGrid(float /*size*/, float /*step*/, float /*minorAlpha*/) {}
    void drawAxes(float /*length*/) {}
    void drawAABB(const glm::vec3 & /*minPt*/, const glm::vec3 & /*maxPt*/, const glm::vec4 & /*color*/) {}
    void drawCross(const glm::vec3 & /*p*/, float /*size*/, const glm::vec4 & /*color*/) {}

  private:
    void ensureInitialized();
    void createShaders();
    void destroyBuffers();
    void uploadMeshToGPU();
    void drawImportedMesh();
    void ensureMeshEdges();

    std::shared_ptr<Scene> m_scene;
    QWidget *m_viewport = nullptr;    // seulement si QWidget
    QOpenGLFunctions *m_gl = nullptr; // fonctions GL de base (GLES/desktop)
    bool m_wireframe = false;
    bool m_meshLoaded = false;
    bool m_buffersDirty = false;
    bool m_isGLES = false;

    std::unique_ptr<QOpenGLShaderProgram> m_program;
    int m_mvpLocation = -1;
    int m_colorLocation = -1;
    glm::mat4 m_viewProj{};

    GLuint m_vertexBuffer = 0;
    GLuint m_triangleBuffer = 0;
    GLuint m_lineBuffer = 0;
    GLsizei m_triangleCount = 0;
    GLsizei m_lineCount = 0;

    std::vector<glm::vec3> m_vertices;
    std::vector<uint32_t> m_triangleIndices;
    std::vector<uint32_t> m_lineIndices;
    AABB m_meshBounds;
    MeshStats m_meshStats;
};
