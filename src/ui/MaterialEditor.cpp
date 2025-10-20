#include "ui/MaterialEditor.h"
#include "core/Scene.h"

MaterialEditor::MaterialEditor(QWidget* parent) : QWidget(parent) {}
MaterialEditor::MaterialEditor(std::shared_ptr<Scene> scene, QWidget* parent)
    : QWidget(parent), m_scene(std::move(scene)) {}
