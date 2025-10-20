#pragma once
#include <QWidget>
#include <memory>
class Scene;
class GeometryEditor : public QWidget {
    Q_OBJECT
  public:
    explicit GeometryEditor(std::shared_ptr<Scene> scene, QWidget *parent = nullptr);
};
