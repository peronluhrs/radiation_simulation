#pragma once
#include <memory>
#include "glm_simple.h"

class Scene;
class QOpenGLWidget; // forward declare: pas besoin d'inclure QOpenGLWidget ici

class Renderer {
public:
    Renderer();
    ~Renderer();

    void attachScene(std::shared_ptr<Scene> s);
    void setViewport(QOpenGLWidget* w); // no-op si OpenGLWidgets indispo
    void renderOnce();                  // no-op placeholder
    void resize(int w, int h);

private:
    std::shared_ptr<Scene> m_scene;
    int m_w{0}, m_h{0};
};
