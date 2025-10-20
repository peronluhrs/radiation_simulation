#include "ui/SourceEditor.h"
#include "core/Scene.h"

SourceEditor::SourceEditor(QWidget* parent) : QWidget(parent) {}
SourceEditor::SourceEditor(std::shared_ptr<Scene> scene, QWidget* parent)
    : QWidget(parent), m_scene(std::move(scene)) {}
