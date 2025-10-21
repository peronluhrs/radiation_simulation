#pragma once

#include <memory>
#include <optional>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <QWidget>
#include <QWindow>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QString>

class Scene;
class MeshObject;

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

    struct MeshImportStats {
        size_t vertexCount = 0;
        size_t faceCount = 0;
    };
    std::optional<MeshImportStats> loadVtk(const QString &filePath, QString *errorMessage = nullptr);

    // Configuration “wireframe” (no-op pour l’instant sous GLES)
    void setWireframe(bool on) { m_wireframe = on; }
    bool wireframe() const { return m_wireframe; }

    // Stubs de dessin (pour compiler ; implémentations à faire plus tard)
    void drawGrid(float /*size*/, float /*step*/, float /*minorAlpha*/) {}
    void drawAxes(float /*length*/) {}
    void drawAABB(const glm::vec3 & /*minPt*/, const glm::vec3 & /*maxPt*/, const glm::vec4 & /*color*/) {}
    void drawCross(const glm::vec3 & /*p*/, float /*size*/, const glm::vec4 & /*color*/) {}

  private:
    void ensureInitialized();

    std::shared_ptr<Scene> m_scene;
    QWidget *m_viewport = nullptr;    // seulement si QWidget
    QOpenGLFunctions *m_gl = nullptr; // fonctions GL de base (GLES/desktop)
    bool m_wireframe = false;
    std::weak_ptr<MeshObject> m_lastImportedMesh;
};
