#include "ui/MaterialEditor.h"
#include <QVBoxLayout>
#include <QLabel>
MaterialEditor::MaterialEditor(std::shared_ptr<Scene>, QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel("Éditeur Matériaux (stub)"));
}
