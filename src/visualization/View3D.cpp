#include "visualization/View3D.h"
#include <cmath>

View3D::View3D(QWindow *parent) : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, parent) {}

void View3D::setScene(std::shared_ptr<Scene> scene) { m_scene = std::move(scene); }

void View3D::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

void View3D::resizeGL(int w, int h) { glViewport(0, 0, w, h); }

void View3D::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // axes simples (mode immédiat, suffisant pour un smoke test)
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    // X (rouge)
    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 0);
    // Y (vert)
    glColor3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 1, 0);
    // Z (bleu)
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 1);
    glEnd();
}
