#pragma once
#include <QWidget>
#include <memory>
class Scene;

class GeometryEditor : public QWidget {
public:
    explicit GeometryEditor(QWidget* parent=nullptr);
    GeometryEditor(std::shared_ptr<Scene> scene, QWidget* parent=nullptr);
private:
    std::shared_ptr<Scene> m_scene;
};
