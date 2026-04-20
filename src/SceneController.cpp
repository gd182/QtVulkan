#include "SceneController.h"
#include "VulkanWindow.h"
#include "VulkanCore.h"

SceneController::SceneController(QObject* parent)
    : QObject(parent)
{}

void SceneController::setElasticity(double e) {
    if (qFuzzyCompare(elasticity, e))
        return;

    elasticity = e;
    emit elasticityChanged();
    if (ownerWindow && ownerWindow->getCoreHandle())
        ownerWindow->getCoreHandle()->setElasticity(static_cast<float>(e));
}

void SceneController::reset() {
    if (ownerWindow && ownerWindow->getCoreHandle())
        ownerWindow->getCoreHandle()->resetPhysics();
}
