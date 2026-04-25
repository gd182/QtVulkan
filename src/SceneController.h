#pragma once

#include <QObject>

class VulkanWindow;

/**
 * @brief Мост между QML и движком Vulkan / Bridge between QML and the Vulkan engine.
 */
class SceneController : public QObject {
    Q_OBJECT

    Q_PROPERTY(double fps READ getFps NOTIFY fpsChanged)
    Q_PROPERTY(double elasticity READ getElasticity NOTIFY elasticityChanged)

public:
    explicit SceneController(QObject* parent = nullptr);

    void setWindow(VulkanWindow* w) { ownerWindow = w; }

    // Геттеры свойств / Property getters
    double getFps() const { return fps; }
    double getElasticity() const { return elasticity; }

    // Вызываются движком / Called by engine
    void setFps(double f) { fps = f; emit fpsChanged(); }

    // Слоты для QML / Slots for QM
    Q_INVOKABLE void setElasticity(double e);
    Q_INVOKABLE void reset();

signals:
    void fpsChanged();
    void elasticityChanged();

private:
    VulkanWindow* ownerWindow = nullptr;
    double fps = 0.0;
    double elasticity = 0.7;
};
