#pragma once
#include <QWidget>
#include <memory>
class Scene;
class SourceEditor : public QWidget {
    Q_OBJECT
  public:
    explicit SourceEditor(std::shared_ptr<Scene> scene, QWidget *parent = nullptr);
};
