#pragma once
#include <QWidget>
#include <memory>
class Scene;

class SensorEditor : public QWidget {
public:
    explicit SensorEditor(QWidget* parent=nullptr);
    SensorEditor(std::shared_ptr<Scene> scene, QWidget* parent=nullptr);
private:
    std::shared_ptr<Scene> m_scene;
};
