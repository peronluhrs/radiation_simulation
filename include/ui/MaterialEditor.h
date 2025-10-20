#pragma once
#include <QWidget>
#include <memory>
class Scene;

class MaterialEditor : public QWidget {
public:
    explicit MaterialEditor(QWidget* parent=nullptr);
    MaterialEditor(std::shared_ptr<Scene> scene, QWidget* parent=nullptr);
private:
    std::shared_ptr<Scene> m_scene;
};
