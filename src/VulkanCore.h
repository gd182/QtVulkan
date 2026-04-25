#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "TypesData.h"
#include "Camera.h"
#include "Physics.h"

namespace vkApp {

/**
 * @brief Ядро рендеринга Vulkan без зависимости от оконной системы
 *        Vulkan rendering core, window-system agnostic.
 */
class VulkanCore {
public:
    explicit VulkanCore(VkInstance instance, VkSurfaceKHR surface,
                        uint32_t widthValue, uint32_t heightValue,
                        std::string shaderDir = "shaders");
    ~VulkanCore();

    VulkanCore(const VulkanCore&) = delete;
    VulkanCore& operator=(const VulkanCore&) = delete;

    // Рендер / Render
    void tick(float dt);
    void resize(uint32_t w, uint32_t h);

    // Управление / Controls
    void zoomCamera(float delta);
    void orbitCamera(float dAzimuth, float dElevation);
    void resetPhysics();
    void setElasticity(float e);

    // Геттеры / Getters
    [[nodiscard]] float getElasticity() const noexcept { return physicsWorld.getElasticity(); }
    [[nodiscard]] uint32_t getWidth()  const noexcept { return width; }
    [[nodiscard]] uint32_t getHeight() const noexcept { return height; }

    // Коллбеки / Callbacks
    void setFpsCallback(std::function<void(double)> cb) { fpsCallback = std::move(cb); }

private:
    std::string shaderDir;

    // Не владеем / Not owned
    VkInstance   instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface  = VK_NULL_HANDLE;
    uint32_t width;
    uint32_t height;

    // Устройства / Devices
    VkPhysicalDevice phys           = VK_NULL_HANDLE;
    VkDevice         device         = VK_NULL_HANDLE;
    VkQueue          graphicsQ      = VK_NULL_HANDLE;
    VkQueue          presentQ       = VK_NULL_HANDLE;
    uint32_t         graphicsFamily = 0;
    uint32_t         presentFamily  = 0;

    // Swapchain
    VkSwapchainKHR           swapchain  = VK_NULL_HANDLE;
    std::vector<VkImage>     scImages;
    std::vector<VkImageView> scViews;
    VkFormat                 swapFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D               extent     = {};

    // Глубина / Depth
    VkImage        depthImg  = VK_NULL_HANDLE;
    VkDeviceMemory depthMem  = VK_NULL_HANDLE;
    VkImageView    depthView = VK_NULL_HANDLE;

    // Render pass & framebuffers
    VkRenderPass                renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer>  fbs;

    // Дескрипторы & UBO
    VkDescriptorSetLayout        dsl   = VK_NULL_HANDLE;
    VkDescriptorPool             dpool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> dsets;
    std::vector<VkBuffer>        uboBuf;
    std::vector<VkDeviceMemory>  uboMem;
    std::vector<void*>           uboMapped;

    // Пайплайн / Pipeline
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline       pipeline       = VK_NULL_HANDLE;

    // Меш шара / Ball mesh
    VkBuffer       vBuf  = VK_NULL_HANDLE;
    VkDeviceMemory vMem  = VK_NULL_HANDLE;
    VkBuffer       iBuf  = VK_NULL_HANDLE;
    VkDeviceMemory iMem  = VK_NULL_HANDLE;
    uint32_t       indexCount = 0;

    // Меш земли / Ground mesh
    VkBuffer       groundVBuf  = VK_NULL_HANDLE;
    VkDeviceMemory groundVMem  = VK_NULL_HANDLE;
    VkBuffer       groundIBuf  = VK_NULL_HANDLE;
    VkDeviceMemory groundIMem  = VK_NULL_HANDLE;
    uint32_t       groundIndexCount = 0;

    // Меш пандуса / Ramp mesh
    VkBuffer       rampVBuf  = VK_NULL_HANDLE;
    VkDeviceMemory rampVMem  = VK_NULL_HANDLE;
    VkBuffer       rampIBuf  = VK_NULL_HANDLE;
    VkDeviceMemory rampIMem  = VK_NULL_HANDLE;
    uint32_t       rampIndexCount = 0;

    // Меш коробки / Box mesh
    VkBuffer       boxVBuf  = VK_NULL_HANDLE;
    VkDeviceMemory boxVMem  = VK_NULL_HANDLE;
    VkBuffer       boxIBuf  = VK_NULL_HANDLE;
    VkDeviceMemory boxIMem  = VK_NULL_HANDLE;
    uint32_t       boxIndexCount = 0;

    // Команды / Commands
    VkCommandPool                cmdPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> cmd;

    // Синхронизация / Sync
    static constexpr int         MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore>     imageAvail;
    std::vector<VkSemaphore>     renderDone;
    std::vector<VkFence>         inFlight;
    std::vector<VkFence>         imagesInFlight;
    int                          currentFrame = 0;

    // Сцена / Scene
    Camera                          camera;
    physics::PhysicsWorld           physicsWorld;
    std::vector<physics::RigidBody> initialBodies; // for reset

    // FPS
    std::chrono::steady_clock::time_point fpsTime;
    int                                   fpsCount = 0;
    std::function<void(double)>           fpsCallback;

    // Отладка / Debug
    VkDebugUtilsMessengerEXT             debugMsgr    = VK_NULL_HANDLE;
    PFN_vkCreateDebugUtilsMessengerEXT   pfnCreateDebug  = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT  pfnDestroyDebug = nullptr;

    // Инициализация / Init
    void initVulkan();
    void setupScene();   // build physicsWorld planes + initial bodies
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createCommandPool();
    void createDepthResources();
    void transitionDepthLayout();
    void createRenderPass();
    void createFramebuffers();
    void createDescriptorSetLayout();
    void createUniformBuffers();
    void createDescriptorPool();
    void updateDescriptorSets();
    void createPipeline();
    void createMeshBuffers();
    void destroyMeshBuffers();
    void createCommandBuffers();
    void createSyncObjects();

    // Кадр / Per-frame
    void drawFrame();
    void recordCommandBuffer(uint32_t i);
    void updateUBO(uint32_t imgIndex);
    void updateFps();

    // Очистка / Cleanup
    void cleanup();
    void recreateSwapchain();

    // Вспомогательные / Helpers
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags props,
                      VkBuffer& buf, VkDeviceMemory& mem);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void createImage(uint32_t w, uint32_t h, VkFormat fmt,
                     VkImageUsageFlags usage,
                     VkImage& img, VkDeviceMemory& mem);
    [[nodiscard]] uint32_t findMemoryType(uint32_t typeBits,
                                          VkMemoryPropertyFlags flags) const;
    VkCommandBuffer beginSingleTime();
    void endSingleTime(VkCommandBuffer cmd);

    static std::vector<uint32_t> readFile(const char* path);
    static VkShaderModule createShaderModule(VkDevice dev,
                                             const std::vector<uint32_t>& code);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* userdata);
};

} // namespace vkApp
