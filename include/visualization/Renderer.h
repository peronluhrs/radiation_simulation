#pragma once

#include <memory>
#include <glm/mat4x4.hpp>

#include <QWidget>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QOpenGLContext>

class Scene;

/**
 * Renderer : petite couche d'abstraction de rendu.
 * - Utilise QOpenGLFunctions (portable / GLES) pour le set de base.
 * - Utilise QOpenGLExtraFunctions (desktop GL) quand dispo (VAO, glPolygonMode, etc.).
 * => En GLES, m_ex == nullptr, on désactive le wireframe et on évite les VAO.
 */
class Renderer {
  public:
    Renderer();
    ~Renderer();

    void setViewport(QWidget *w);                   // QWidget (container du viewport)
    void attachScene(std::shared_ptr<Scene> scene); // attacher une scène
    void resize(int w, int h);                      // informer le viewport size
    void beginFrame(const glm::mat4 &viewProj);     // clear + setup
    void endFrame();                                // draw helpers, restore states
    void renderOnce();                              // trigger repaint du widget

    // Optionnel : API pour wireframe
    void setWireframe(bool on) { m_wireframe = on; }
    bool wireframe() const { return m_wireframe; }

  private:
    void ensureInitialized();

    std::shared_ptr<Scene> m_scene;
    QWidget *m_viewport = nullptr;

    QOpenGLFunctions *m_gl = nullptr;      // fonctions GL de base
    QOpenGLExtraFunctions *m_ex = nullptr; // fonctions desktop (peut être null)
    unsigned int m_vao = 0;                // VAO si desktop GL

    bool m_wireframe = false;
};
