#pragma once

#include <memory>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <QWidget>
#include <QOpenGLFunctions>
#include <QOpenGLContext>

class Scene;

/**
 * Renderer minimal et compatible GLES :
 * - Utilise uniquement QOpenGLFunctions (pas de VAO, pas de glPolygonMode).
 * - Fournit les méthodes attendues par View3D (stubs no-op pour le moment).
 *   => Codex pourra implémenter drawGrid/drawAxes/drawAABB/drawCross plus tard
 *      en VBO/shaders, sans fixed pipeline ni immediate mode.
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
    void initialize(QWidget *w); // setViewport + ensureInitialized
    void setScene(std::shared_ptr<Scene> s) { attachScene(std::move(s)); }
    void setWireframeEnabled(bool on) { setWireframe(on); }

    // Configuration “wireframe” (no-op pour l’instant sous GLES)
    void setWireframe(bool on) { m_wireframe = on; }
    bool wireframe() const { return m_wireframe; }

    // Stubs de dessin (pour compiler ; implémentation à faire par Codex)
    void drawGrid(float /*size*/, float /*step*/, float /*minorAlpha*/) {}
    void drawAxes(float /*length*/) {}
    void drawAABB(const glm::vec3 & /*minPt*/, const glm::vec3 & /*maxPt*/, const glm::vec3 & /*color*/) {}
    void drawCross(const glm::vec3 & /*p*/, float /*size*/, const glm::vec3 & /*color*/) {}

  private:
    void ensureInitialized();

    std::shared_ptr<Scene> m_scene;
    QWidget *m_viewport = nullptr;

    QOpenGLFunctions *m_gl = nullptr; // fonctions GL de base (toujours disponibles)
    bool m_wireframe = false;
};
