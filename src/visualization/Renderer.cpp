#include "visualization/Renderer.h"
#include "core/Scene.h"

#include <QOpenGLContext>
#include <QSurfaceFormat>

// -----------------------------------------------------------------------------
// Cette implé simple :
// - initialise m_gl (toujours) et m_ex (si desktop GL).
// - en desktop GL: VAO + glPolygonMode (wireframe).
// - en GLES: pas de VAO, wireframe désactivé (sans casser le build).
// -----------------------------------------------------------------------------

Renderer::Renderer() = default;

Renderer::~Renderer() {
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

    if (m_ex) {
        m_ex->glPolygonMode(GL_FRONT_AND_BACK, m_wireframe ? GL_LINE : GL_FILL);
    } else {
        // En GLES, glPolygonMode n'existe pas -> on reste en FILL
        if (m_wireframe)
            m_wireframe = false;
    }
}

void Renderer::endFrame() {
    ensureInitialized();
    if (!m_gl)
        return;

    // Si desktop GL: on peut binder un VAO (ex: axes/grille)
    if (m_ex)
        m_ex->glBindVertexArray(m_vao);

    // TODO: draw helpers ici (axes/grille), ou via un autre module

    if (m_ex)
        m_ex->glBindVertexArray(0);

    // Restaurer le polygon mode par défaut si dispo
    if (m_ex)
        m_ex->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::renderOnce() {
    if (m_viewport)
        m_viewport->update();
}

void Renderer::ensureInitialized() {
    if (m_gl)
        return;

    if (auto *ctx = QOpenGLContext::currentContext()) {
        m_gl = ctx->functions();      // set de base (existe aussi en GLES)
        m_ex = ctx->extraFunctions(); // desktop GL uniquement ; null si GLES
    }
    if (!m_gl)
        return;

    if (m_ex) {
        // Desktop GL : créer un VAO basique
        m_ex->glGenVertexArrays(1, &m_vao);
        m_ex->glBindVertexArray(m_vao);
        // ... init éventuels buffers/index/attribs ...
        m_ex->glBindVertexArray(0);
    } else {
        // GLES: pas de VAO
        m_vao = 0;
    }
}
