#include "visualization/Renderer.h"
#include "visualization/VtkLegacyLoader.h"
#include "io/VtkLegacyLoader.h"
#if __has_include(<GL/gl.h>)
#include <GL/gl.h>
#endif
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

bool Renderer::loadVtk(const std::string &path, std::string *err) {
    VtkMesh raw;
    std::string e;
    if (!VtkLegacyLoader::load(path, raw, &e)) {
        if (err)
            *err = e;
        return false;
    }
    m_mesh.verts.clear();
    m_mesh.tris.clear();
    m_mesh.lines.clear();
    m_mesh.verts.reserve(raw.vertices.size());
    for (auto &p : raw.vertices)
        m_mesh.verts.push_back(glm::vec3(p[0], p[1], p[2]));
    m_mesh.tris.reserve(raw.triangles.size());
    for (auto &t : raw.triangles)
        m_mesh.tris.push_back({t[0], t[1], t[2]});
    m_mesh.lines.reserve(raw.lines.size());
    for (auto &l : raw.lines)
        m_mesh.lines.push_back({l[0], l[1]});
    return true;
}

void Renderer::drawLoadedMesh() {
    if (m_mesh.empty())
        return;
#if __has_include(<GL/gl.h>)
    // Triangles pleins (gris clair)
    glDisable(GL_CULL_FACE);
    glBegin(GL_TRIANGLES);
    glColor3f(0.8f, 0.82f, 0.85f);
    for (auto &t : m_mesh.tris) {
        auto &a = m_mesh.verts[t[0]];
        auto &b = m_mesh.verts[t[1]];
        auto &c = m_mesh.verts[t[2]];
        glVertex3f(a.x, a.y, a.z);
        glVertex3f(b.x, b.y, b.z);
        glVertex3f(c.x, c.y, c.z);
    }
    glEnd();

    // Arêtes (noires)
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glColor3f(0.1f, 0.1f, 0.1f);
    for (auto &e : m_mesh.lines) {
        auto &a = m_mesh.verts[e[0]];
        auto &b = m_mesh.verts[e[1]];
        glVertex3f(a.x, a.y, a.z);
        glVertex3f(b.x, b.y, b.z);
    }
    glEnd();
#endif
}
