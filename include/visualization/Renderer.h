#include "io/VtkLegacyLoader.h"
#pragma once

#include <memory>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <QWidget>
#include <QWindow>
#include <QOpenGLFunctions>
#include <QOpenGLContext>

class Scene;

/**
 * Renderer minimal et compatible GLES :
 * - Utilise uniquement QOpenGLFunctions (pas de VAO, pas de glPolygonMode).
 * - Fournit les méthodes attendues par View3D (stubs no-op pour le moment).
 */
class Renderer {
  public:
    bool loadVtk(const std::string &path, std::string *err = nullptr);
    void drawLoadedMesh();
    struct LoadedMesh {
        std::vector<glm::vec3> verts;
        std::vector<std::array<int, 3>> tris;
        std::vector<std::array<int, 2>> lines;
        bool empty() const { return verts.empty(); }
    };

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

    // Stubs de dessin (pour compiler ; implémentations à faire plus tard)
    void drawGrid(float /*size*/, float /*step*/, float /*minorAlpha*/) {}
    void drawAxes(float /*length*/) {}
    void drawAABB(const glm::vec3 & /*minPt*/, const glm::vec3 & /*maxPt*/, const glm::vec4 & /*color*/) {}
    void drawCross(const glm::vec3 & /*p*/, float /*size*/, const glm::vec4 & /*color*/) {}

  private:
    VtkMesh m_vtkMesh; // maillage importé VTK
    LoadedMesh m_mesh;
    void ensureInitialized();

    std::shared_ptr<Scene> m_scene;
    QWidget *m_viewport = nullptr;    // seulement si QWidget
    QOpenGLFunctions *m_gl = nullptr; // fonctions GL de base (GLES/desktop)
    bool m_wireframe = false;
};
