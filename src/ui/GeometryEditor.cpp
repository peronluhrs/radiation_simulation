#include "ui/GeometryEditor.h"
#include <QVBoxLayout>
#include <QLabel>
GeometryEditor::GeometryEditor(std::shared_ptr<Scene>, QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel("Éditeur Géométrie (stub)"));
}
