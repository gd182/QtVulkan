#include <QDir>
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

    // Ensure relative paths (e.g. "shaders/") resolve correctly on all platforms,
    // including inside macOS .app bundles where the OS sets CWD to "/".
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    QVulkanInstance inst;
    // Vulkan 1.1 promotes VK_KHR_get_physical_device_properties2 to core, which is
    // required by MoltenVK's VK_KHR_portability_subset device extension on macOS.
    inst.setApiVersion(QVersionNumber(1, 1));
#ifndef NDEBUG
    inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
#endif

    QByteArrayList extensions;
#ifndef NDEBUG
    extensions << "VK_EXT_debug_utils";
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    extensions << "VK_KHR_portability_enumeration";
#endif
    inst.setExtensions(extensions);

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
