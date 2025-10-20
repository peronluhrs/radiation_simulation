#pragma once
#include <QWidget>
#include <memory>
class Scene;

class SourceEditor : public QWidget {
public:
    explicit SourceEditor(QWidget* parent=nullptr);
    SourceEditor(std::shared_ptr<Scene> scene, QWidget* parent=nullptr);
private:
    std::shared_ptr<Scene> m_scene;
};
