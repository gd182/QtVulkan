#include "VulkanCore.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "vk3dSphere.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// Вспомогательные типы / Helper types
namespace {

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    [[nodiscard]] bool complete() const {
        return graphics.has_value() && present.has_value();
    }
};

struct SwapSupport {
    VkSurfaceCapabilitiesKHR        caps{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

[[noreturn]] static void die(const char* msg) {
    std::fprintf(stderr, "Fatal: %s\n", msg);
    std::exit(EXIT_FAILURE);
}

static void vkCheck(VkResult r, const char* what) {
    if (r != VK_SUCCESS) {
        std::fprintf(stderr, "Vulkan error %d at: %s\n", static_cast<int>(r), what);
        std::exit(EXIT_FAILURE);
    }
}

static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    QueueFamilyIndices idx;
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, props.data());

    for (uint32_t i = 0; i < count; ++i) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            idx.graphics = i;
        VkBool32 presentOk = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentOk);
        if (presentOk) idx.present = i;
        if (idx.complete()) break;
    }
    return idx;
}

static SwapSupport querySwapSupport(VkPhysicalDevice dev, VkSurfaceKHR surface) {
    SwapSupport s;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &s.caps);
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &count, nullptr);
    s.formats.resize(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &count, s.formats.data());
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &count, nullptr);
    s.presentModes.resize(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &count, s.presentModes.data());
    return s;
}

static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& fmts) {
    for (const auto& f : fmts)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    return fmts[0];
}

static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (auto m : modes)
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, uint32_t w, uint32_t h) {
    if (caps.currentExtent.width != UINT32_MAX)
        return caps.currentExtent;
    VkExtent2D e{};
    e.width = std::clamp(w, caps.minImageExtent.width,  caps.maxImageExtent.width);
    e.height = std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height);
    return e;
}

} // anonymous namespace

namespace vkApp {

// Конструктор / Constructor
VulkanCore::VulkanCore(VkInstance instance, VkSurfaceKHR surface,
                       uint32_t width, uint32_t height,
                       std::string shaderDir)
    : shaderDir(std::move(shaderDir))
    , instance(instance)
    , surface(surface)
    , width(width)
    , height(height)
{
    setupScene();
    initVulkan();
    fpsTime = std::chrono::steady_clock::now();
}

VulkanCore::~VulkanCore() {
    cleanup();
}

// Публичный API / Public API

void VulkanCore::zoomCamera(float delta) {
    camera.zoom(delta);
}

void VulkanCore::orbitCamera(float dAzimuth, float dElevation) {
    camera.orbit(dAzimuth, dElevation);
}

void VulkanCore::resetPhysics() {
    physicsWorld.resetBodies(initialBodies);
    camera.reset();
}

void VulkanCore::setElasticity(float e) {
    physicsWorld.setElasticity(e);
}

// Основной тик / Main tick

void VulkanCore::tick(float dt) {
    physicsWorld.update(dt);
    drawFrame();
    updateFps();
}

// Инициализация / Initialisation

static constexpr float kGroundY = physics::PhysicsWorld::kGroundY;

void VulkanCore::setupScene()
{
    using namespace physics;
    const float gy = kGroundY;

    // ── Collision planes ──────────────────────────────────────────────────

    // Flat ground
    CollisionPlane ground;
    ground.point       = glm::vec3(0.0f, gy, 0.0f);
    ground.normal      = glm::vec3(0.0f, 1.0f, 0.0f);
    ground.restitution = 0.6f;
    ground.friction    = 0.5f;
    physicsWorld.addPlane(ground);

    // Ramp: 25° incline rising in +X, foot of ramp at x=0, y=gy
    // Normal = (-sin25°, cos25°, 0)
    const float sinA = 0.4226f, cosA = 0.9063f;
    CollisionPlane ramp;
    ramp.point       = glm::vec3(0.0f, gy, 0.0f);
    ramp.normal      = glm::vec3(-sinA, cosA, 0.0f);
    ramp.restitution = 0.35f;
    ramp.friction    = 0.55f;
    ramp.bounded     = true;
    ramp.boundsMin   = glm::vec3( 0.0f, -10.0f, -3.6f);
    ramp.boundsMax   = glm::vec3( 4.5f,  10.0f,  3.6f);
    physicsWorld.addPlane(ramp);

    // ── Rigid bodies ──────────────────────────────────────────────────────

    // Sphere — drops onto the ramp
    RigidBody sphere;
    sphere.shape       = ShapeType::Sphere;
    sphere.position    = glm::vec3(1.5f, 4.0f, 0.0f);
    sphere.radius      = 0.5f;
    sphere.mass        = 1.0f;
    sphere.restitution = 0.6f;
    sphere.friction    = 0.5f;

    // Box — drops onto the flat ground with a slight initial tilt
    RigidBody box;
    box.shape       = ShapeType::Box;
    box.position    = glm::vec3(-1.5f, 5.0f, 0.3f);
    box.halfExtents = glm::vec3(0.4f, 0.4f, 0.4f);
    box.mass        = 1.5f;
    box.restitution = 0.4f;
    box.friction    = 0.6f;
    box.orientation = glm::angleAxis(glm::radians(20.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    initialBodies = {sphere, box};
    physicsWorld.resetBodies(initialBodies);
}

void VulkanCore::initVulkan() {
    pfnCreateDebug = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    pfnDestroyDebug = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    setupDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createCommandPool();
    createDepthResources();
    transitionDepthLayout();
    createRenderPass();
    createFramebuffers();
    createDescriptorSetLayout();
    createUniformBuffers();
    createDescriptorPool();
    updateDescriptorSets();
    createPipeline();
    createMeshBuffers();
    createCommandBuffers();
    createSyncObjects();
}

void VulkanCore::setupDebugMessenger() {
    if (!pfnCreateDebug) return;

    VkDebugUtilsMessengerCreateInfoEXT di{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    di.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    di.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    di.pfnUserCallback = debugCallback;

    vkCheck(pfnCreateDebug(instance, &di, nullptr, &debugMsgr),
            "vkCreateDebugUtilsMessengerEXT");
}

void VulkanCore::pickPhysicalDevice() {
    uint32_t count = 0;
    vkCheck(vkEnumeratePhysicalDevices(instance, &count, nullptr),
            "vkEnumeratePhysicalDevices(count)");
    if (count == 0) die("No Vulkan-capable GPU found");

    std::vector<VkPhysicalDevice> devs(count);
    vkCheck(vkEnumeratePhysicalDevices(instance, &count, devs.data()),
            "vkEnumeratePhysicalDevices(list)");

    for (auto d : devs) {
        auto idx = findQueueFamilies(d, surface);
        if (!idx.complete()) continue;
        auto ss = querySwapSupport(d, surface);
        if (ss.formats.empty() || ss.presentModes.empty()) continue;
        phys = d;
        graphicsFamily = idx.graphics.value();
        presentFamily = idx.present.value();
        break;
    }

    if (phys == VK_NULL_HANDLE)
        die("No suitable GPU (requires graphics + present + swapchain)");
}

void VulkanCore::createLogicalDevice() {
    float pri = 1.0f;
    std::vector<uint32_t> families = { graphicsFamily };
    if (presentFamily != graphicsFamily)
        families.push_back(presentFamily);

    std::vector<VkDeviceQueueCreateInfo> qcis;
    for (uint32_t qf : families) {
        VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        qci.queueFamilyIndex = qf;
        qci.queueCount       = 1;
        qci.pQueuePriorities = &pri;
        qcis.push_back(qci);
    }

    std::vector<const char*> devExts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // VK_KHR_portability_subset must be enabled when the device advertises it.
    // MoltenVK (macOS) always exposes it; ignoring it triggers a validation error.
    {
        uint32_t n = 0;
        vkEnumerateDeviceExtensionProperties(phys, nullptr, &n, nullptr);
        std::vector<VkExtensionProperties> available(n);
        vkEnumerateDeviceExtensionProperties(phys, nullptr, &n, available.data());
        for (const auto& e : available) {
            if (std::strcmp(e.extensionName, "VK_KHR_portability_subset") == 0) {
                devExts.push_back("VK_KHR_portability_subset");
                break;
            }
        }
    }

    VkPhysicalDeviceFeatures feats{};
    VkDeviceCreateInfo dci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    dci.queueCreateInfoCount    = static_cast<uint32_t>(qcis.size());
    dci.pQueueCreateInfos       = qcis.data();
    dci.enabledExtensionCount   = static_cast<uint32_t>(devExts.size());
    dci.ppEnabledExtensionNames = devExts.data();
    dci.pEnabledFeatures        = &feats;

    vkCheck(vkCreateDevice(phys, &dci, nullptr, &device), "vkCreateDevice");
    vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQ);
    vkGetDeviceQueue(device, presentFamily,  0, &presentQ);
}

void VulkanCore::createSwapchain() {
    auto ss     = querySwapSupport(phys, surface);
    auto sf     = chooseSurfaceFormat(ss.formats);
    auto pm     = choosePresentMode(ss.presentModes);
    extent     = chooseExtent(ss.caps, width, height);
    swapFormat = sf.format;

    uint32_t imgCount = ss.caps.minImageCount + 1;
    if (ss.caps.maxImageCount > 0)
        imgCount = std::min(imgCount, ss.caps.maxImageCount);

    VkSwapchainCreateInfoKHR sci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    sci.surface          = surface;
    sci.minImageCount    = imgCount;
    sci.imageFormat      = sf.format;
    sci.imageColorSpace  = sf.colorSpace;
    sci.imageExtent      = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.preTransform     = ss.caps.currentTransform;
    sci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode      = pm;
    sci.clipped          = VK_TRUE;

    if (graphicsFamily != presentFamily) {
        uint32_t qf[2] = { graphicsFamily, presentFamily };
        sci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices   = qf;
    } else {
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    vkCheck(vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain),
            "vkCreateSwapchainKHR");

    uint32_t n = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &n, nullptr);
    scImages.resize(n);
    vkGetSwapchainImagesKHR(device, swapchain, &n, scImages.data());

    scViews.resize(n);
    for (uint32_t i = 0; i < n; ++i) {
        VkImageViewCreateInfo iv{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        iv.image                       = scImages[i];
        iv.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        iv.format                      = swapFormat;
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.layerCount = 1;
        vkCheck(vkCreateImageView(device, &iv, nullptr, &scViews[i]),
                "vkCreateImageView(color)");
    }
}

void VulkanCore::createCommandPool() {
    VkCommandPoolCreateInfo cpci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    cpci.queueFamilyIndex = graphicsFamily;
    cpci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCheck(vkCreateCommandPool(device, &cpci, nullptr, &cmdPool),
            "vkCreateCommandPool");
}

void VulkanCore::createDepthResources() {
    constexpr VkFormat depthFmt = VK_FORMAT_D32_SFLOAT;
    createImage(extent.width, extent.height, depthFmt,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                depthImg, depthMem);

    VkImageViewCreateInfo iv{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    iv.image                       = depthImg;
    iv.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    iv.format                      = depthFmt;
    iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    iv.subresourceRange.levelCount = 1;
    iv.subresourceRange.layerCount = 1;
    vkCheck(vkCreateImageView(device, &iv, nullptr, &depthView),
            "vkCreateImageView(depth)");
}

void VulkanCore::transitionDepthLayout() {
    VkCommandBuffer cmd = beginSingleTime();

    VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    b.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    b.newLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image               = depthImg;
    b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    b.subresourceRange.levelCount = 1;
    b.subresourceRange.layerCount = 1;
    b.srcAccessMask       = 0;
    b.dstAccessMask       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        0, 0, nullptr, 0, nullptr, 1, &b);

    endSingleTime(cmd);
}

void VulkanCore::createRenderPass() {
    VkAttachmentDescription colorAtt{};
    colorAtt.format        = swapFormat;
    colorAtt.samples       = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp       = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAtt.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAtt{};
    depthAtt.format        = VK_FORMAT_D32_SFLOAT;
    depthAtt.samples       = VK_SAMPLE_COUNT_1_BIT;
    depthAtt.loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.storeOp       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAtt.finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription sub{};
    sub.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount    = 1;
    sub.pColorAttachments       = &colorRef;
    sub.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> atts = { colorAtt, depthAtt };
    VkRenderPassCreateInfo rpci{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpci.attachmentCount = static_cast<uint32_t>(atts.size());
    rpci.pAttachments    = atts.data();
    rpci.subpassCount    = 1;
    rpci.pSubpasses      = &sub;
    rpci.dependencyCount = 1;
    rpci.pDependencies   = &dep;

    vkCheck(vkCreateRenderPass(device, &rpci, nullptr, &renderPass),
            "vkCreateRenderPass");
}

void VulkanCore::createFramebuffers() {
    fbs.resize(scImages.size());
    for (uint32_t i = 0; i < static_cast<uint32_t>(scImages.size()); ++i) {
        std::array<VkImageView, 2> views = { scViews[i], depthView };

        VkFramebufferCreateInfo fci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fci.renderPass = renderPass;
        fci.attachmentCount = static_cast<uint32_t>(views.size());
        fci.pAttachments = views.data();
        fci.width = extent.width;
        fci.height = extent.height;
        fci.layers= 1;

        vkCheck(vkCreateFramebuffer(device, &fci, nullptr, &fbs[i]),
                "vkCreateFramebuffer");
    }
}

void VulkanCore::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding         = 0;
    uboBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    ci.bindingCount = 1;
    ci.pBindings    = &uboBinding;

    vkCheck(vkCreateDescriptorSetLayout(device, &ci, nullptr, &dsl),
            "vkCreateDescriptorSetLayout");
}

void VulkanCore::createUniformBuffers() {
    const uint32_t n = static_cast<uint32_t>(scImages.size());
    uboBuf.resize(n);
    uboMem.resize(n);
    uboMapped.resize(n, nullptr);

    for (uint32_t i = 0; i < n; ++i) {
        createBuffer(sizeof(typesData::UBO),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uboBuf[i], uboMem[i]);
        vkCheck(vkMapMemory(device, uboMem[i], 0, sizeof(typesData::UBO), 0, &uboMapped[i]),
                "vkMapMemory(ubo)");
    }
}

void VulkanCore::createDescriptorPool() {
    const uint32_t n = static_cast<uint32_t>(scImages.size());

    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = n;

    VkDescriptorPoolCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    ci.poolSizeCount = 1;
    ci.pPoolSizes    = &poolSize;
    ci.maxSets       = n;

    vkCheck(vkCreateDescriptorPool(device, &ci, nullptr, &dpool),
            "vkCreateDescriptorPool");

    dsets.resize(n);
    std::vector<VkDescriptorSetLayout> layouts(n, dsl);

    VkDescriptorSetAllocateInfo ai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    ai.descriptorPool     = dpool;
    ai.descriptorSetCount = n;
    ai.pSetLayouts        = layouts.data();

    vkCheck(vkAllocateDescriptorSets(device, &ai, dsets.data()),
            "vkAllocateDescriptorSets");
}

void VulkanCore::updateDescriptorSets() {
    for (uint32_t i = 0; i < static_cast<uint32_t>(dsets.size()); ++i) {
        VkDescriptorBufferInfo bi{};
        bi.buffer = uboBuf[i];
        bi.offset = 0;
        bi.range  = sizeof(typesData::UBO);

        VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        w.dstSet          = dsets[i];
        w.dstBinding      = 0;
        w.descriptorCount = 1;
        w.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w.pBufferInfo     = &bi;

        vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
    }
}

void VulkanCore::createPipeline() {
    auto vertCode = readFile((shaderDir + "/sphere.vert.spv").c_str());
    auto fragCode = readFile((shaderDir + "/sphere.frag.spv").c_str());

    VkShaderModule vert = createShaderModule(device, vertCode);
    VkShaderModule frag = createShaderModule(device, fragCode);

    VkPipelineShaderStageCreateInfo vs{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    vs.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vs.module = vert;
    vs.pName  = "main";

    VkPipelineShaderStageCreateInfo fs{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    fs.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fs.module = frag;
    fs.pName  = "main";

    std::array<VkPipelineShaderStageCreateInfo, 2> stages = { vs, fs };

    VkVertexInputBindingDescription bind{};
    bind.binding   = 0;
    bind.stride    = sizeof(typesData::Vertex);
    bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attr{};
    attr[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                static_cast<uint32_t>(offsetof(typesData::Vertex, pos)) };
    attr[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT,
                static_cast<uint32_t>(offsetof(typesData::Vertex, nor)) };
    attr[2] = { 2, 0, VK_FORMAT_R32G32B32_SFLOAT,
                static_cast<uint32_t>(offsetof(typesData::Vertex, col)) };

    VkPipelineVertexInputStateCreateInfo vis{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vis.vertexBindingDescriptionCount   = 1;
    vis.pVertexBindingDescriptions      = &bind;
    vis.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr.size());
    vis.pVertexAttributeDescriptions    = attr.data();

    VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vps{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    vps.viewportCount = 1;
    vps.scissorCount  = 1;

    constexpr VkDynamicState dynStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dyn.dynamicStateCount = static_cast<uint32_t>(std::size(dynStates));
    dyn.pDynamicStates    = dynStates;

    VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode    = VK_CULL_MODE_BACK_BIT;
    rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    ds.depthTestEnable  = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp   = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState cbAtt{};
    cbAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    cb.attachmentCount = 1;
    cb.pAttachments    = &cbAtt;

    // Push-константа: матрица модели (64 байта) для каждого вызова draw
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo plci{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    plci.setLayoutCount         = 1;
    plci.pSetLayouts            = &dsl;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges    = &pcRange;

    vkCheck(vkCreatePipelineLayout(device, &plci, nullptr, &pipelineLayout),
            "vkCreatePipelineLayout");

    VkGraphicsPipelineCreateInfo gpci{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    gpci.stageCount          = static_cast<uint32_t>(stages.size());
    gpci.pStages             = stages.data();
    gpci.pVertexInputState   = &vis;
    gpci.pInputAssemblyState = &ia;
    gpci.pViewportState      = &vps;
    gpci.pRasterizationState = &rs;
    gpci.pMultisampleState   = &ms;
    gpci.pDepthStencilState  = &ds;
    gpci.pColorBlendState    = &cb;
    gpci.pDynamicState       = &dyn;
    gpci.layout              = pipelineLayout;
    gpci.renderPass          = renderPass;
    gpci.subpass             = 0;

    vkCheck(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpci, nullptr, &pipeline),
            "vkCreateGraphicsPipelines");

    vkDestroyShaderModule(device, vert, nullptr);
    vkDestroyShaderModule(device, frag, nullptr);
}

void VulkanCore::createMeshBuffers() {
    // Шар / Ball
    {
        std::vector<typesData::Vertex> verts;
        std::vector<uint32_t>          inds;
        // Радиус 1.0; масштаб ball.radius применяется через матрицу модели
        vkObj::Vk3dSphere::makeUVSphere(20, 40, 1.0f, {0.2f, 0.6f, 1.0f}, verts, inds);
        indexCount = static_cast<uint32_t>(inds.size());

        const VkDeviceSize vSize = verts.size() * sizeof(typesData::Vertex);
        const VkDeviceSize iSize = inds.size()  * sizeof(uint32_t);

        VkBuffer       vS{}, iS{};
        VkDeviceMemory vSM{}, iSM{};
        createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     vS, vSM);
        createBuffer(iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     iS, iSM);

        void* p = nullptr;
        vkMapMemory(device, vSM, 0, vSize, 0, &p); std::memcpy(p, verts.data(), vSize); vkUnmapMemory(device, vSM);
        vkMapMemory(device, iSM, 0, iSize, 0, &p); std::memcpy(p, inds.data(),  iSize); vkUnmapMemory(device, iSM);

        createBuffer(vSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vBuf, vMem);
        createBuffer(iSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, iBuf, iMem);
        copyBuffer(vS, vBuf, vSize);
        copyBuffer(iS, iBuf, iSize);
        vkDestroyBuffer(device, vS, nullptr); vkFreeMemory(device, vSM, nullptr);
        vkDestroyBuffer(device, iS, nullptr); vkFreeMemory(device, iSM, nullptr);
    }

    // Helper: stage + upload a static mesh to device-local buffers
    auto uploadMesh = [&](const std::vector<typesData::Vertex>& verts,
                          const std::vector<uint32_t>&          inds,
                          VkBuffer& outVBuf, VkDeviceMemory& outVMem,
                          VkBuffer& outIBuf, VkDeviceMemory& outIMem,
                          uint32_t& outIndexCount)
    {
        outIndexCount = static_cast<uint32_t>(inds.size());
        const VkDeviceSize vSize = verts.size() * sizeof(typesData::Vertex);
        const VkDeviceSize iSize = inds.size()  * sizeof(uint32_t);
        VkBuffer vS{}, iS{}; VkDeviceMemory vSM{}, iSM{};
        createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vS, vSM);
        createBuffer(iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, iS, iSM);
        void* p = nullptr;
        vkMapMemory(device, vSM, 0, vSize, 0, &p); std::memcpy(p, verts.data(), vSize); vkUnmapMemory(device, vSM);
        vkMapMemory(device, iSM, 0, iSize, 0, &p); std::memcpy(p, inds.data(),  iSize); vkUnmapMemory(device, iSM);
        createBuffer(vSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outVBuf, outVMem);
        createBuffer(iSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outIBuf, outIMem);
        copyBuffer(vS, outVBuf, vSize); copyBuffer(iS, outIBuf, iSize);
        vkDestroyBuffer(device, vS, nullptr); vkFreeMemory(device, vSM, nullptr);
        vkDestroyBuffer(device, iS, nullptr); vkFreeMemory(device, iSM, nullptr);
    };

    // Земля / Ground plane  (CCW for normal (0,1,0), grey-green)
    {
        const float gy = kGroundY, ext = 4.0f;
        const float cr = 0.3f, cg = 0.45f, cb = 0.3f;
        std::vector<typesData::Vertex> verts = {
            {{ -ext, gy, -ext }, { 0.f, 1.f, 0.f }, { cr, cg, cb }},
            {{ -ext, gy, ext }, { 0.f, 1.f, 0.f }, { cr, cg, cb }},
            {{  ext, gy,  ext }, { 0.f, 1.f, 0.f }, { cr, cg, cb }},
            {{ ext, gy,  -ext }, { 0.f, 1.f, 0.f }, { cr, cg, cb }},
        };
        std::vector<uint32_t> inds = { 0, 1, 2, 0, 2, 3 };
        uploadMesh(verts, inds, groundVBuf, groundVMem, groundIBuf, groundIMem, groundIndexCount);
    }

    // Пандус / Ramp  (25° incline rising in +X, normal = (-sin25°, cos25°, 0), sandy)
    {
        const float tanA = 0.4663f, sinA = 0.4226f, cosA = 0.9063f;
        const float x0 = 0.0f, x1 = 4.0f;
        const float y0 = kGroundY, y1 = kGroundY + (x1 - x0) * tanA;
        const float zE = 3.5f;
        const float cr = 0.75f, cg = 0.55f, cb = 0.3f;
        std::vector<typesData::Vertex> verts = {
            {{ x0, y0, -zE }, { -sinA, cosA, 0.f }, { cr, cg, cb }},
            {{ x0, y0,  zE }, { -sinA, cosA, 0.f }, { cr, cg, cb }},
            {{ x1, y1,  zE }, { -sinA, cosA, 0.f }, { cr, cg, cb }},
            {{ x1, y1, -zE }, { -sinA, cosA, 0.f }, { cr, cg, cb }},
        };
        std::vector<uint32_t> inds = { 0, 1, 2, 0, 2, 3 };
        uploadMesh(verts, inds, rampVBuf, rampVMem, rampIBuf, rampIMem, rampIndexCount);
    }

    // Единичный куб / Unit box  (±1 local space, scaled by halfExtents in model matrix, red)
    {
        const float cr = 0.85f, cg = 0.3f, cb = 0.2f;
        std::vector<typesData::Vertex> verts;
        std::vector<uint32_t>          inds;

        // addFace: 4 corners + outward normal → 4 verts + 2 triangles (CCW winding verified)
        auto addFace = [&](float ax, float ay, float az,
                           float bx, float by, float bz,
                           float cx, float cy, float cz,
                           float dx, float dy, float dz,
                           float nx, float ny, float nz)
        {
            const uint32_t base = static_cast<uint32_t>(verts.size());
            verts.push_back({{ ax, ay, az }, { nx, ny, nz }, { cr, cg, cb }});
            verts.push_back({{ bx, by, bz }, { nx, ny, nz }, { cr, cg, cb }});
            verts.push_back({{ cx, cy, cz }, { nx, ny, nz }, { cr, cg, cb }});
            verts.push_back({{ dx, dy, dz }, { nx, ny, nz }, { cr, cg, cb }});
            inds.insert(inds.end(), { base, base+1, base+2, base, base+2, base+3 });
        };

        addFace(-1,1,-1, -1,1,1,  1,1,1,  1,1,-1,   0, 1, 0); // +Y
        addFace( 1,-1,-1, 1,-1,1,-1,-1,1,-1,-1,-1,   0,-1, 0); // -Y
        addFace( 1,-1,-1, 1,1,-1, 1,1,1,  1,-1,1,    1, 0, 0); // +X
        addFace(-1,-1,1,-1,1,1, -1,1,-1,-1,-1,-1,   -1, 0, 0); // -X
        addFace(-1,-1,1,  1,-1,1, 1,1,1, -1,1,1,    0, 0, 1);  // +Z
        addFace( 1,-1,-1,-1,-1,-1,-1,1,-1,1,1,-1,   0, 0,-1);  // -Z

        uploadMesh(verts, inds, boxVBuf, boxVMem, boxIBuf, boxIMem, boxIndexCount);
    }
}

void VulkanCore::destroyMeshBuffers() {
    auto destroy = [&](VkBuffer& b, VkDeviceMemory& m) {
        if (b) { vkDestroyBuffer(device, b, nullptr); vkFreeMemory(device, m, nullptr); }
        b = VK_NULL_HANDLE; m = VK_NULL_HANDLE;
    };
    destroy(iBuf,       iMem);
    destroy(vBuf,       vMem);
    destroy(groundIBuf, groundIMem);
    destroy(groundVBuf, groundVMem);
    destroy(rampIBuf,   rampIMem);
    destroy(rampVBuf,   rampVMem);
    destroy(boxIBuf,    boxIMem);
    destroy(boxVBuf,    boxVMem);
}

void VulkanCore::createCommandBuffers() {
    cmd.resize(scImages.size());

    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.commandPool        = cmdPool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = static_cast<uint32_t>(cmd.size());

    vkCheck(vkAllocateCommandBuffers(device, &ai, cmd.data()),
            "vkAllocateCommandBuffers");
}

void VulkanCore::createSyncObjects() {
    VkSemaphoreCreateInfo semCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceCI{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    imageAvail.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    inFlight.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    renderDone.assign(scImages.size(), VK_NULL_HANDLE);
    imagesInFlight.assign(scImages.size(), VK_NULL_HANDLE);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkCheck(vkCreateSemaphore(device, &semCI,   nullptr, &imageAvail[i]), "vkCreateSemaphore(imageAvail)");
        vkCheck(vkCreateFence    (device, &fenceCI, nullptr, &inFlight[i]),   "vkCreateFence");
    }

    for (size_t i = 0; i < renderDone.size(); ++i)
        vkCheck(vkCreateSemaphore(device, &semCI, nullptr, &renderDone[i]), "vkCreateSemaphore(renderDone)");
}

// Пофреймовые операции / Per-frame operations

void VulkanCore::drawFrame() {
    if (width == 0 || height == 0) return;

    vkCheck(vkWaitForFences(device, 1, &inFlight[currentFrame], VK_TRUE, UINT64_MAX),
            "vkWaitForFences");

    uint32_t imgIndex = 0;
    VkResult acq = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                                         imageAvail[currentFrame], VK_NULL_HANDLE, &imgIndex);
    if (acq == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    vkCheck(acq, "vkAcquireNextImageKHR");

    if (imagesInFlight[imgIndex] != VK_NULL_HANDLE) {
        vkCheck(vkWaitForFences(device, 1, &imagesInFlight[imgIndex], VK_TRUE, UINT64_MAX),
                "vkWaitForFences(imageInFlight)");
    }
    imagesInFlight[imgIndex] = inFlight[currentFrame];

    vkCheck(vkResetFences(device, 1, &inFlight[currentFrame]),
            "vkResetFences");

    updateUBO(imgIndex);

    vkCheck(vkResetCommandBuffer(cmd[imgIndex], 0), "vkResetCommandBuffer");
    recordCommandBuffer(imgIndex);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = &imageAvail[currentFrame];
    si.pWaitDstStageMask    = &waitStage;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &cmd[imgIndex];
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = &renderDone[imgIndex];

    vkCheck(vkQueueSubmit(graphicsQ, 1, &si, inFlight[currentFrame]),
            "vkQueueSubmit");

    VkPresentInfoKHR pi{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &renderDone[imgIndex];
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &swapchain;
    pi.pImageIndices      = &imgIndex;

    VkResult pr = vkQueuePresentKHR(presentQ, &pi);
    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else {
        vkCheck(pr, "vkQueuePresentKHR");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanCore::recordCommandBuffer(uint32_t i) {
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkCheck(vkBeginCommandBuffer(cmd[i], &bi), "vkBeginCommandBuffer");

    VkClearValue clears[2]{};
    clears[0].color        = { { 0.05f, 0.05f, 0.08f, 1.0f } };
    clears[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rbi{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rbi.renderPass = renderPass;
    rbi.framebuffer = fbs[i];
    rbi.renderArea.extent = extent;
    rbi.clearValueCount = 2;
    rbi.pClearValues = clears;

    vkCmdBeginRenderPass(cmd[i], &rbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport vp{};
    vp.width    = static_cast<float>(extent.width);
    vp.height   = static_cast<float>(extent.height);
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport(cmd[i], 0, 1, &vp);

    VkRect2D sc{};
    sc.extent = extent;
    vkCmdSetScissor(cmd[i], 0, 1, &sc);

    // Дескрипторный набор (viewProj + lightDir) одинаков для обоих объектов
    vkCmdBindDescriptorSets(cmd[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &dsets[i], 0, nullptr);

    VkDeviceSize off = 0;

    // Вспомогательная лямбда для отрисовки объекта / Helper to draw one object
    auto drawObject = [&](VkBuffer vb, VkBuffer ib, uint32_t idxCount, const glm::mat4& model) {
        vkCmdPushConstants(cmd[i], pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
        vkCmdBindVertexBuffers(cmd[i], 0, 1, &vb, &off);
        vkCmdBindIndexBuffer  (cmd[i], ib, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd[i], idxCount, 1, 0, 0, 0);
    };

    // Земля / Ground  (world-space verts → identity model)
    drawObject(groundVBuf, groundIBuf, groundIndexCount, glm::mat4(1.0f));

    // Пандус / Ramp  (world-space verts → identity model)
    drawObject(rampVBuf, rampIBuf, rampIndexCount, glm::mat4(1.0f));

    // Тела / Physics bodies
    const auto& bodies = physicsWorld.getBodies();
    if (bodies.size() >= 1) {
        // Сфера / Sphere
        const auto& ball = bodies[0];
        glm::mat4 model = glm::translate(glm::mat4(1.0f), ball.position)
                        * glm::mat4_cast(ball.orientation)
                        * glm::scale(glm::mat4(1.0f), glm::vec3(ball.radius));
        drawObject(vBuf, iBuf, indexCount, model);
    }
    if (bodies.size() >= 2) {
        // Коробка / Box
        const auto& box = bodies[1];
        glm::mat4 model = glm::translate(glm::mat4(1.0f), box.position)
                        * glm::mat4_cast(box.orientation)
                        * glm::scale(glm::mat4(1.0f), box.halfExtents);
        drawObject(boxVBuf, boxIBuf, boxIndexCount, model);
    }

    vkCmdEndRenderPass(cmd[i]);
    vkCheck(vkEndCommandBuffer(cmd[i]), "vkEndCommandBuffer");
}

void VulkanCore::updateUBO(uint32_t imgIndex) {
    const float aspect = (extent.height > 0)
        ? static_cast<float>(extent.width) / static_cast<float>(extent.height)
        : 1.0f;

    glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
    proj[1][1] *= -1.0f; // Vulkan Y-flip

    typesData::UBO ubo{};
    ubo.viewProj = proj * camera.viewMatrix();
    ubo.lightDir = glm::vec4(0.4f, 0.7f, 0.2f, 0.0f);

    std::memcpy(uboMapped[imgIndex], &ubo, sizeof(typesData::UBO));
}

void VulkanCore::updateFps() {
    ++fpsCount;
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - fpsTime).count();
    if (elapsed >= 1.0) {
        if (fpsCallback) fpsCallback(fpsCount / elapsed);
        fpsCount = 0;
        fpsTime  = now;
    }
}

// Resize / Recreate swapchain

void VulkanCore::resize(uint32_t w, uint32_t h) {
    width  = w;
    height = h;
    recreateSwapchain();
}

void VulkanCore::recreateSwapchain() {
    if (width == 0 || height == 0) return;

    vkDeviceWaitIdle(device);

    for (VkSemaphore semaphore : renderDone) {
        if (semaphore) vkDestroySemaphore(device, semaphore, nullptr);
    }
    renderDone.clear();
    imagesInFlight.clear();

    if (!cmd.empty()) {
        vkFreeCommandBuffers(device, cmdPool,
            static_cast<uint32_t>(cmd.size()), cmd.data());
        cmd.clear();
    }
    for (auto fb : fbs) vkDestroyFramebuffer(device, fb, nullptr);
    fbs.clear();

    if (depthView) { vkDestroyImageView(device, depthView, nullptr); depthView = VK_NULL_HANDLE; }
    if (depthImg) { vkDestroyImage (device, depthImg, nullptr); depthImg = VK_NULL_HANDLE; }
    if (depthMem) { vkFreeMemory (device, depthMem, nullptr); depthMem = VK_NULL_HANDLE; }

    for (auto v : scViews) vkDestroyImageView(device, v, nullptr);
    scViews.clear();
    scImages.clear();

    if (swapchain) { vkDestroySwapchainKHR(device, swapchain, nullptr); swapchain = VK_NULL_HANDLE; }

    createSwapchain();
    createDepthResources();
    transitionDepthLayout();
    createFramebuffers();
    createCommandBuffers();
    renderDone.assign(scImages.size(), VK_NULL_HANDLE);
    imagesInFlight.assign(scImages.size(), VK_NULL_HANDLE);
    VkSemaphoreCreateInfo semCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    for (size_t i = 0; i < renderDone.size(); ++i)
        vkCheck(vkCreateSemaphore(device, &semCI, nullptr, &renderDone[i]), "vkCreateSemaphore(renderDone)");

    currentFrame = 0;
}

// Очистка / Cleanup

void VulkanCore::cleanup() {
    if (device == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device);

    for (VkSemaphore semaphore : imageAvail) {
        if (semaphore) vkDestroySemaphore(device, semaphore, nullptr);
    }
    for (VkSemaphore semaphore : renderDone) {
        if (semaphore) vkDestroySemaphore(device, semaphore, nullptr);
    }
    for (VkFence fence : inFlight) {
        if (fence) vkDestroyFence(device, fence, nullptr);
    }

    destroyMeshBuffers();

    for (size_t i = 0; i < uboBuf.size(); ++i) {
        if (uboBuf[i]) vkDestroyBuffer(device, uboBuf[i], nullptr);
        if (uboMem[i]) vkFreeMemory (device, uboMem[i], nullptr);
    }

    if (dpool) vkDestroyDescriptorPool (device, dpool, nullptr);
    if (dsl)   vkDestroyDescriptorSetLayout(device, dsl, nullptr);

    for (auto fb : fbs) vkDestroyFramebuffer(device, fb, nullptr);

    if (pipeline) vkDestroyPipeline (device, pipeline, nullptr);
    if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    if (renderPass) vkDestroyRenderPass (device, renderPass, nullptr);

    if (depthView) vkDestroyImageView(device, depthView, nullptr);
    if (depthImg) vkDestroyImage (device, depthImg,  nullptr);
    if (depthMem) vkFreeMemory (device, depthMem,  nullptr);

    for (auto v : scViews) vkDestroyImageView(device, v, nullptr);
    if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);

    if (cmdPool) vkDestroyCommandPool(device, cmdPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (pfnDestroyDebug && debugMsgr)
        pfnDestroyDebug(instance, debugMsgr, nullptr);
}

// Вспомогательные функции / Helpers

void VulkanCore::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags props,
                               VkBuffer& buf, VkDeviceMemory& mem)
{
    VkBufferCreateInfo ci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    ci.size        = size;
    ci.usage       = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCheck(vkCreateBuffer(device, &ci, nullptr, &buf), "vkCreateBuffer");

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(device, buf, &req);

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);
    vkCheck(vkAllocateMemory(device, &ai, nullptr, &mem), "vkAllocateMemory");
    vkCheck(vkBindBufferMemory(device, buf, mem, 0), "vkBindBufferMemory");
}

void VulkanCore::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBuffer cmd = beginSingleTime();
    VkBufferCopy c{};
    c.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &c);
    endSingleTime(cmd);
}

void VulkanCore::createImage(uint32_t w, uint32_t h, VkFormat fmt,
                              VkImageUsageFlags usage,
                              VkImage& img, VkDeviceMemory& mem)
{
    VkImageCreateInfo ii{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.extent = { w, h, 1 };
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.format = fmt;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ii.usage = usage;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCheck(vkCreateImage(device, &ii, nullptr, &img), "vkCreateImage");

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(device, img, &req);

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize  = req.size;
    ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkCheck(vkAllocateMemory(device, &ai, nullptr, &mem), "vkAllocateMemory(image)");
    vkCheck(vkBindImageMemory(device, img, mem, 0), "vkBindImageMemory");
}

uint32_t VulkanCore::findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags flags) const {
    VkPhysicalDeviceMemoryProperties mp{};
    vkGetPhysicalDeviceMemoryProperties(phys, &mp);

    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
        if ((typeBits & (1u << i)) &&
            ((mp.memoryTypes[i].propertyFlags & flags) == flags))
            return i;
    }
    throw std::runtime_error("No suitable Vulkan memory type found");
}

VkCommandBuffer VulkanCore::beginSingleTime() {
    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.commandPool        = cmdPool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    VkCommandBuffer cmd{};
    vkCheck(vkAllocateCommandBuffers(device, &ai, &cmd), "vkAllocateCommandBuffers(single)");

    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkCheck(vkBeginCommandBuffer(cmd, &bi), "vkBeginCommandBuffer(single)");
    return cmd;
}

void VulkanCore::endSingleTime(VkCommandBuffer cmd) {
    vkCheck(vkEndCommandBuffer(cmd), "vkEndCommandBuffer(single)");

    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    vkCheck(vkQueueSubmit(graphicsQ, 1, &si, VK_NULL_HANDLE), "vkQueueSubmit(single)");
    vkQueueWaitIdle(graphicsQ);
    vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
}

std::vector<uint32_t> VulkanCore::readFile(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        std::fprintf(stderr, "Cannot open file: %s\n", path);
        std::exit(EXIT_FAILURE);
    }
    const std::streamsize size = f.tellg();
    if (size <= 0 || (size % 4) != 0) {
        std::fprintf(stderr, "Invalid SPIR-V size in file: %s\n", path);
        std::exit(EXIT_FAILURE);
    }

    std::vector<uint32_t> buf(static_cast<size_t>(size / 4));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

VkShaderModule VulkanCore::createShaderModule(VkDevice dev, const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ci.codeSize = code.size() * sizeof(uint32_t);
    ci.pCode    = code.data();
    VkShaderModule m{};
    vkCheck(vkCreateShaderModule(dev, &ci, nullptr, &m), "vkCreateShaderModule");
    return m;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanCore::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*)
{
    std::fprintf(stderr, "[Vulkan Validation] %s\n", data->pMessage);
    return VK_FALSE;
}

} // namespace vkApp
