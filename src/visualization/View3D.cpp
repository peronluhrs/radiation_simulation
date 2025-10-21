#include "visualization/View3D.h"

#include <cmath>
#include <algorithm>

#include "core/Scene.h"
#include "core/Sensor.h"
#include "core/Source.h"
#include "geometry/Object3D.h"

#include <QMouseEvent>
#include <QWheelEvent>

View3D::View3D(QWindow *parent) : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, parent) {}

void View3D::setScene(std::shared_ptr<Scene> scene) {
    m_scene = std::move(scene);
    if (m_renderer)
        m_renderer->setScene(m_scene);
    update();
}

std::optional<View3D::MeshImportStats> View3D::importVtkMesh(const QString &filePath, QString *errorMessage) {
    if (!m_renderer) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Renderer non initialisé.");
        return std::nullopt;
    }

    auto stats = m_renderer->loadVtk(filePath, errorMessage);
    if (!stats.has_value())
        return std::nullopt;

    update();
    return MeshImportStats{stats->vertexCount, stats->faceCount};
}

void View3D::setWireframeEnabled(bool enabled) {
    m_wireframe = enabled;
    if (m_renderer)
        m_renderer->setWireframeEnabled(enabled);
    update();
}

void View3D::setShowSensors(bool enabled) {
    m_showSensors = enabled;
    update();
}

void View3D::setShowSources(bool enabled) {
    m_showSources = enabled;
    update();
}

void View3D::resetCamera() {
    m_target = glm::vec3(0.0f);
    m_distance = 8.0f;
    m_yaw = glm::radians(135.0f);
    m_pitch = glm::radians(35.0f);
    update();
}

void View3D::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    m_renderer = std::make_unique<Renderer>();
    m_renderer->initialize(this);
    m_renderer->setScene(m_scene);
    m_renderer->setWireframeEnabled(m_wireframe);
}

void View3D::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    if (m_renderer)
        m_renderer->resize(w, h);
}

void View3D::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!m_renderer)
        return;

    glm::mat4 view = buildViewMatrix();
    glm::mat4 proj = buildProjectionMatrix();
    glm::mat4 viewProj = proj * view;

    m_renderer->beginFrame(viewProj);
    m_renderer->drawGrid(20.0f, 1.0f, 0.25f);
    m_renderer->drawAxes(1.5f);
    updateSceneHelpers();
    m_renderer->endFrame();
}

void View3D::mousePressEvent(QMouseEvent *event) {
    m_activeButton = event->button();
    updateCursorTracking(event->pos());
    event->accept();
}

void View3D::mouseMoveEvent(QMouseEvent *event) {
    if (m_activeButton == Qt::NoButton) {
        event->ignore();
        return;
    }

    QPoint pos = event->pos();
    QPoint delta = pos - m_lastPos;
    m_lastPos = pos;

    if (m_activeButton == Qt::LeftButton) {
        float rotationSpeed = 0.005f;
        m_yaw -= delta.x() * rotationSpeed;
        m_pitch -= delta.y() * rotationSpeed;
        float limit = glm::radians(85.0f);
        m_pitch = std::clamp(m_pitch, -limit, limit);
        update();
    } else if (m_activeButton == Qt::MiddleButton) {
        float panSpeed = std::max(0.01f, m_distance * 0.002f);
        glm::vec3 camPos = cameraPosition();
        glm::vec3 forward = glm::normalize(m_target - camPos);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, forward));

        m_target -= right * static_cast<float>(delta.x()) * panSpeed;
        m_target += up * static_cast<float>(delta.y()) * panSpeed;
        update();
    }

    event->accept();
}

void View3D::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == m_activeButton)
        m_activeButton = Qt::NoButton;
    event->accept();
}

void View3D::wheelEvent(QWheelEvent *event) {
    float steps = static_cast<float>(event->angleDelta().y()) / 120.0f;
    float zoomFactor = std::pow(0.9f, steps);
    m_distance = std::clamp(m_distance * zoomFactor, 1.0f, 250.0f);
    update();
    event->accept();
}

void View3D::mouseDoubleClickEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    resetCamera();
}

glm::mat4 View3D::buildViewMatrix() const {
    glm::vec3 eye = cameraPosition();
    return glm::lookAt(eye, m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 View3D::buildProjectionMatrix() const {
    int w = std::max(1, width());
    int h = std::max(1, height());
    float aspect = static_cast<float>(w) / static_cast<float>(h);
    return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 500.0f);
}

glm::vec3 View3D::cameraPosition() const {
    float cosPitch = std::cos(m_pitch);
    glm::vec3 offset{
        m_distance * cosPitch * std::cos(m_yaw),
        m_distance * std::sin(m_pitch),
        m_distance * cosPitch * std::sin(m_yaw)
    };
    return m_target + offset;
}

void View3D::updateCursorTracking(const QPoint &pos) {
    m_lastPos = pos;
}

void View3D::updateSceneHelpers() {
    if (!m_scene || !m_renderer)
        return;

    const auto &objects = m_scene->getAllObjects();
    for (const auto &object : objects) {
        if (!object || !object->isVisible())
            continue;
        const AABB &bounds = object->getBounds();
        if (!bounds.isValid())
            continue;

        glm::vec4 color(object->getColor(), 1.0f);
        if (object->isSelected())
            color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
        m_renderer->drawAABB(bounds.min, bounds.max, color);
    }

    if (m_showSensors) {
        const auto &sensors = m_scene->getAllSensors();
        for (const auto &sensor : sensors) {
            if (!sensor || !sensor->isVisible())
                continue;
            glm::vec4 color(sensor->getColor(), 1.0f);
            m_renderer->drawCross(sensor->getPosition(), 0.5f, color);
        }
    }

    if (m_showSources) {
        const auto &sources = m_scene->getAllSources();
        for (const auto &source : sources) {
            if (!source || !source->isVisible())
                continue;
            glm::vec4 color(source->getColor(), 1.0f);
            m_renderer->drawCross(source->getPosition(), 0.7f, color);
        }
    }
}
