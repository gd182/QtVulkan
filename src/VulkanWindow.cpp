#include "VulkanWindow.h"
#include "VulkanCore.h"
#include "SceneController.h"

#include <QCoreApplication>
#include <QExposeEvent>
#include <QPlatformSurfaceEvent>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>

// Конструктор / Constructor

VulkanWindow::VulkanWindow(QVulkanInstance* inst, QWindow* parent)
    : QWindow(parent)
    , qvkInst(inst)
{
    setSurfaceType(QSurface::VulkanSurface);
    setVulkanInstance(inst);

    controllerHandle = new SceneController(this);

    renderTimer = new QTimer(this);
    renderTimer->setInterval(17);
    connect(renderTimer, &QTimer::timeout, this, &VulkanWindow::renderFrame);
}

VulkanWindow::~VulkanWindow() {
    if (renderTimer)
        renderTimer->stop();
    delete coreHandle;
    coreHandle = nullptr;
}

bool VulkanWindow::event(QEvent* event) {
    if (event->type() == QEvent::PlatformSurface) {
        auto* surfaceEvent = static_cast<QPlatformSurfaceEvent*>(event);
        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            delete coreHandle;
            coreHandle = nullptr;
            initialized = false;
        }
    }

    return QWindow::event(event);
}

// exposeEvent

void VulkanWindow::exposeEvent(QExposeEvent* /*event*/) {
    if (isExposed() && !initialized)
        init();
}

// resizeEvent

void VulkanWindow::resizeEvent(QResizeEvent* event) {
    QWindow::resizeEvent(event);
    if (!coreHandle || width() == 0 || height() == 0) return;

    renderTimer->stop();
    coreHandle->resize(static_cast<uint32_t>(width()),
                  static_cast<uint32_t>(height()));
    renderTimer->start();
}

// init

void VulkanWindow::init() {
    initialized = true;

    VkSurfaceKHR surface = qvkInst->surfaceForWindow(this);

    const std::string shaderDir =
        (QCoreApplication::applicationDirPath() + "/shaders").toStdString();

    coreHandle = new vkApp::VulkanCore(
        qvkInst->vkInstance(),
        surface,
        static_cast<uint32_t>(width()),
        static_cast<uint32_t>(height()),
        shaderDir
    );

    coreHandle->setFpsCallback([this](double fpsValue) {
        controllerHandle->setFps(fpsValue);
    });

    // Синхронизировать начальную упругость / Sync initial elasticity
    coreHandle->setElasticity(static_cast<float>(controllerHandle->getElasticity()));

    controllerHandle->setWindow(this);

    frameTimer.start();
    renderTimer->start();
}

// renderFrame

void VulkanWindow::renderFrame() {
    if (!coreHandle) return;

    float dt = static_cast<float>(frameTimer.elapsed()) / 1000.0f;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.016f;
    frameTimer.restart();

    coreHandle->tick(dt);
}

// Клавиатура / Keyboard

void VulkanWindow::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) return;
    switch (event->key()) {
    case Qt::Key_R:
    case Qt::Key_Space:
        if (coreHandle) coreHandle->resetPhysics();
        break;
    default:
        QWindow::keyPressEvent(event);
        break;
    }
}

// Мышь / Mouse

void VulkanWindow::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        lmbPressed = true;
        lastMousePos = event->position();
    }
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton)
        lmbPressed = false;
}

void VulkanWindow::mouseMoveEvent(QMouseEvent* event) {
    if (!lmbPressed || !coreHandle) return;
    const QPointF ballPos = event->position();
    const float dx = static_cast<float>(ballPos.x() - lastMousePos.x());
    const float dy = static_cast<float>(ballPos.y() - lastMousePos.y());
    lastMousePos = ballPos;
    coreHandle->orbitCamera(dx * kOrbitSensitivity, -dy * kOrbitSensitivity);
}

void VulkanWindow::wheelEvent(QWheelEvent* event) {
    if (!coreHandle) return;
    const float notches = static_cast<float>(event->angleDelta().y()) / 120.0f;
    coreHandle->zoomCamera(notches * kScrollSensitivity);
}
