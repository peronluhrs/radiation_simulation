#include "visualization/Renderer.h"
#include "core/Scene.h"

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::attachScene(std::shared_ptr<Scene> s) { m_scene = std::move(s); }
void Renderer::setViewport(QOpenGLWidget*) { /* no-op */ }
void Renderer::renderOnce() { /* no-op: placeholder pour la GUI */ }
void Renderer::resize(int w, int h) { m_w = w; m_h = h; }
