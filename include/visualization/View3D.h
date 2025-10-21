#pragma once
#include <memory>

#if __has_include(<QOpenGLWindow>) && __has_include(<QOpenGLFunctions>)
#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QPoint>
#else
class QOpenGLWindow;
class QOpenGLFunctions;
class QPoint;
#endif

#include "glm_simple.h"
#if __has_include(<QOpenGLWindow>) && __has_include(<QOpenGLFunctions>)
#include "visualization/Renderer.h"
#else
class Renderer;
#endif

class QString;
class Scene;

class View3D : public QOpenGLWindow, protected QOpenGLFunctions {
    Q_OBJECT
  public:
    explicit View3D(QWindow *parent = nullptr);
    ~View3D() override = default;

    void setScene(std::shared_ptr<Scene> scene);

    void setWireframeEnabled(bool enabled);
    void setShowSensors(bool enabled);
    void setShowSources(bool enabled);
    void resetCamera();
    bool importVtkMesh(const QString &filePath, std::string *err = nullptr);

  protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

  private:
    glm::mat4 buildViewMatrix() const;
    glm::mat4 buildProjectionMatrix() const;
    glm::vec3 cameraPosition() const;
    void updateCursorTracking(const QPoint &pos);
    void updateSceneHelpers();

    std::shared_ptr<Scene> m_scene;
    std::unique_ptr<Renderer> m_renderer;

    glm::vec3 m_target{0.0f};
    float m_distance = 8.0f;
    float m_yaw = glm::radians(135.0f);
    float m_pitch = glm::radians(35.0f);

    QPoint m_lastPos;
    Qt::MouseButton m_activeButton = Qt::NoButton;

    bool m_wireframe = false;
    bool m_showSensors = true;
    bool m_showSources = true;
};
