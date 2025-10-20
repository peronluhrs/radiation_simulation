#pragma once
#include <memory>
#if __has_include(<QOpenGLWindow>) && __has_include(<QOpenGLFunctions>)
#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#else
class QOpenGLWindow;
class QOpenGLFunctions;
#endif

class Scene;

class View3D : public QOpenGLWindow, protected QOpenGLFunctions {
    Q_OBJECT
  public:
    explicit View3D(QWindow *parent = nullptr);
    ~View3D() override = default;

    void setScene(std::shared_ptr<Scene> scene);

  protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

  private:
    std::shared_ptr<Scene> m_scene;
};
