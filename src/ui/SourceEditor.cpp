#include "ui/SourceEditor.h"
#include <QVBoxLayout>
#include <QLabel>
SourceEditor::SourceEditor(std::shared_ptr<Scene>, QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel("Éditeur Sources (stub)"));
}
