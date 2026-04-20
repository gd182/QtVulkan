#pragma once

#include <QWindow>
#include <QVulkanInstance>
#include <QElapsedTimer>
#include <QPointF>

class QTimer;
class SceneController;

namespace vkApp { class VulkanCore; }

/**
 * @brief Qt-окно Vulkan-рендеринга / Qt-based Vulkan render window
 */
class VulkanWindow : public QWindow {
    Q_OBJECT

public:
    explicit VulkanWindow(QVulkanInstance* inst, QWindow* parent = nullptr);
    ~VulkanWindow() override;

    SceneController* getControllerHandle() const { return controllerHandle;}
    vkApp::VulkanCore* getCoreHandle() const { return coreHandle;}

protected:
    bool event(QEvent* event) override;
    void exposeEvent(QExposeEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void renderFrame();

private:
    void init();

    QVulkanInstance* qvkInst = nullptr;
    vkApp::VulkanCore* coreHandle = nullptr;
    SceneController* controllerHandle = nullptr;
    QTimer* renderTimer = nullptr;
    QElapsedTimer frameTimer;
    bool initialized = false;

    // Мышь / Mouse
    bool lmbPressed = false;
    QPointF lastMousePos;

    // Константы / Constants
    static constexpr float kOrbitSensitivity = 0.005f;
    static constexpr float kScrollSensitivity = 0.4f;
};
