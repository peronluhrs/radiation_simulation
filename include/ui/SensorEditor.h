#pragma once
#include <QWidget>
#include <memory>
class Scene;
class SensorEditor : public QWidget {
    Q_OBJECT
  public:
    explicit SensorEditor(std::shared_ptr<Scene> scene, QWidget *parent = nullptr);
};
