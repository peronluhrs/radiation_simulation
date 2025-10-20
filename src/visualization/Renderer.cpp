#include "visualization/Renderer.h"
#include "core/Scene.h"

#include <QOpenGLContext>
#include <QWidget>

// -----------------------------------------------------------------------------
// NOTE: Cette implémentation utilise QOpenGLFunctions pour le "core" (portable)
// et QOpenGLExtraFunctions pour les appels Desktop GL (VAO, glPolygonMode).
// Sous GLES, m_ex sera null: on bypass VAO et on désactive le wireframe.
// -----------------------------------------------------------------------------

Renderer::Renderer() = default;

Renderer::~Renderer() {
    // VAO = API desktop GL (extra). Sous GLES m_ex est null -> rien à détruire.
    if (m_ex && m_vao) {
        m_ex->glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void Renderer::setViewport(QWidget *w) { m_viewport = w; }

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

    // Wireframe via ExtraFunctions (glPolygonMode n’existe pas en GLES)
    if (m_ex) {
        m_ex->glPolygonMode(GL_FRONT_AND_BACK, m_wireframe ? GL_LINE : GL_FILL);
    } else {
        // Pas de wireframe en GLES
        if (m_wireframe)
            m_wireframe = false;
    }
}

void Renderer::endFrame() {
    ensureInitialized();
    if (!m_gl)
        return;

    // Exemple d’usage d’un VAO si dispo (desktop GL)
    if (m_ex)
        m_ex->glBindVertexArray(m_vao);
    // … ici viendront les draw calls réels (axes, grille, etc.) …
    if (m_ex)
        m_ex->glBindVertexArray(0);

    // Rétablir le polygon mode par défaut si dispo
    if (m_ex) {
        m_ex->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Renderer::renderOnce() {
    if (m_viewport)
        m_viewport->update();
}

void Renderer::ensureInitialized() {
    if (m_gl)
        return;

    if (auto *ctx = QOpenGLContext::currentContext()) {
        m_gl = ctx->functions();      // core (portable)
        m_ex = ctx->extraFunctions(); // desktop extra (peut être null sous GLES)
    }
    if (!m_gl)
        return;

    // VAO uniquement si extraFunctions dispo (desktop GL)
    if (m_ex) {
        m_ex->glGenVertexArrays(1, &m_vao);
        m_ex->glBindVertexArray(m_vao);
        // … init buffers si besoin …
        m_ex->glBindVertexArray(0);
    } else {
        m_vao = 0; // pas de VAO en GLES
    }
}
