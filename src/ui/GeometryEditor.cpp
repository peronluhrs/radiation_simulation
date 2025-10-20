#include "ui/GeometryEditor.h"
#include "core/Scene.h"

GeometryEditor::GeometryEditor(QWidget* parent) : QWidget(parent) {}
GeometryEditor::GeometryEditor(std::shared_ptr<Scene> scene, QWidget* parent)
    : QWidget(parent), m_scene(std::move(scene)) {}
