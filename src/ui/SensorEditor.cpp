#include "ui/SensorEditor.h"
#include "core/Scene.h"

SensorEditor::SensorEditor(QWidget* parent) : QWidget(parent) {}
SensorEditor::SensorEditor(std::shared_ptr<Scene> scene, QWidget* parent)
    : QWidget(parent), m_scene(std::move(scene)) {}
