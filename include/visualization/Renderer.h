#pragma once
#include <memory>
#include <QWidget>

class Scene;

class Renderer {
  public:
    Renderer() = default;
    ~Renderer() = default;

    inline void attachScene(std::shared_ptr<Scene> scene) { m_scene = std::move(scene); }
    inline void setViewport(QWidget *w) { m_viewport = w; }
    inline void resize(int w, int h) {
        m_w = w;
        m_h = h;
    }
    inline void renderOnce() {
        // no-op pour l’instant — si on veut juste forcer un repaint :
        if (m_viewport)
            m_viewport->update();
    }

  private:
    std::shared_ptr<Scene> m_scene;
    QWidget *m_viewport{nullptr};
    int m_w{0}, m_h{0};
};
