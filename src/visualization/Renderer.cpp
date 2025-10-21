#include "visualization/Renderer.h"
#include "core/Scene.h"

#include <QOpenGLContext>
#include <QSurfaceFormat>

// Implémentation minimaliste, compatible GLES :
// - pas de VAO / glPolygonMode
// - clear + depth + viewport uniquement
// - méthodes “attendues” par View3D sont présentes (stubs)

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::setViewport(QWidget *w) { m_viewport = w; }

void Renderer::initialize(QWidget *w) {
    setViewport(w);
    ensureInitialized();
}

void Renderer::initialize(QWindow * /*win*/) {
    // View3D est un QOpenGLWindow -> pas de QWidget à stocker
    // On se contente de récupérer les fonctions à partir du contexte courant.
    ensureInitialized();
}

void Renderer::attachScene(std::shared_ptr<Scene> scene) { m_scene = std::move(scene); }

void Renderer::resize(int w, int h) {
    ensureInitialized();
    if (!m_gl)
        return;
    m_gl->glViewport(0, 0, w, h);
}

void Renderer::beginFrame(const glm::mat4 & /*viewProj*/) {
    ensureInitialized();
    if (!m_gl)
        return;

    m_gl->glEnable(GL_DEPTH_TEST);
    m_gl->glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    m_gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // NOTE: Pas de glPolygonMode (non présent en GLES). Le “wireframe” sera
    // géré plus tard via un chemin shader basé sur des lignes (Codex task).
}

void Renderer::endFrame() {
    ensureInitialized();
    if (!m_gl)
        return;

    // Ici viendront les draw calls (grille, axes, etc.) lorsque Codex les ajoutera
    // via VBOs / GL_LINES compatibles GLES et desktop.
}

void Renderer::renderOnce() {
    if (m_viewport)
        m_viewport->update();
}

void Renderer::ensureInitialized() {
    if (m_gl)
        return;

    if (auto *ctx = QOpenGLContext::currentContext()) {
        m_gl = ctx->functions(); // set de base (fonctionne en GLES et desktop)
    }
    // Si m_gl est null, on attend le prochain appel (viewport pas prêt)
}
