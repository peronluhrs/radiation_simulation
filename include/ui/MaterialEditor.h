#pragma once
#include <QWidget>
#include <memory>
class Scene;
class MaterialEditor : public QWidget {
    Q_OBJECT
  public:
    explicit MaterialEditor(std::shared_ptr<Scene> scene, QWidget *parent = nullptr);
};
