#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QVersionNumber>
#include <QVulkanInstance>

#include "src/VulkanWindow.h"
#include "src/SceneController.h"

int main(int argc, char* argv[])
{
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);

    QVulkanInstance inst;
    inst.setApiVersion(QVersionNumber(1, 0));
    inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
    inst.setExtensions({ "VK_EXT_debug_utils" });
    if (!inst.create())
        qFatal("Failed to create Vulkan instance");

    VulkanWindow vulkanWindow(&inst);
    vulkanWindow.setTitle("Vulkan 3D Viewer");
    vulkanWindow.resize(960, 540);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("sceneController", vulkanWindow.getControllerHandle());
    engine.rootContext()->setContextProperty("renderWindow", &vulkanWindow);
    engine.loadFromModule("QtVulkan", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
