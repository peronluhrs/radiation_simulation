#include "ui/SensorEditor.h"
#include <QVBoxLayout>
#include <QLabel>
SensorEditor::SensorEditor(std::shared_ptr<Scene>, QWidget *parent) : QWidget(parent) {
    auto *lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel("Éditeur Capteurs (stub)"));
}
