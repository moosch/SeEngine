#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;

const char* WIN_TITLE = "SeEngine";
const u32 WIN_WIDTH = 800;
const u32 WIN_HEIGHT = 600;

const u32 validationLayerCount = 1;
const char *validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
const u32 deviceExtensionCount = 1;
const char *deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

typedef struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  u32 formatCount;
  VkSurfaceFormatKHR *formats;
  u32 presentModeCount;
  VkPresentModeKHR *presentModes;
} SwapChainSupportDetails;

typedef struct QueueFamilyIndices {
  u32 graphicsFamily;
  bool isGraphicsFamilySet;
  u32 presentFamily;
  bool isPresentFamilySet;
} QueueFamilyIndices;

typedef struct App {
  GLFWwindow *window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice;
  QueueFamilyIndices queueFamilyIndices;
  VkDevice device; // Logical device
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  VkSwapchainKHR swapChain;
  u32 swapChainImageCount;
  VkImage *swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  VkImageView *swapChainImageViews;
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
} App;

void initWindow(App *pApp);
void initVulkan(App *pApp);
void mainLoop(App *pApp);
void cleanup(App *pApp);

void createInstance(App *pApp);

void createSurface(App *app);

bool checkValidationLayerSupport(void);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);

// TODO: Move to own source file
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL) {
    func(instance, debugMessenger, pAllocator);
  }
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *createInfo);

void setupDebugMessenger(App *pApp);

void pickPhysicalDevice(App *pApp);

u32 rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

void createLogicalDevice(App *pApp);

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

VkPresentModeKHR chooseSwapPresentMode(u32 presentModeCount, VkPresentModeKHR *availablePresentModes);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(u32 formatCount, VkSurfaceFormatKHR *availableFormats);
VkExtent2D chooseSwapExtent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities);

void createSwapChain(App *pApp);

void createImageViews(App *pApp);

void createRenderPass(App *pApp);

void createGraphicsPipeline(App *pApp);

u32 clamp_u32(u32 n, u32 min, u32 max);

int main(void) {
  App app = {0};

  initWindow(&app);
  initVulkan(&app);
  mainLoop(&app);
  cleanup(&app);

  return 0;
}

void initWindow(App *pApp) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  pApp->window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE, NULL, NULL);
}

void initVulkan(App *pApp) {
  createInstance(pApp);
  setupDebugMessenger(pApp);
  createSurface(pApp);
  pickPhysicalDevice(pApp);
  createLogicalDevice(pApp);
  createSwapChain(pApp);
  createImageViews(pApp);
  createRenderPass(pApp);
  createGraphicsPipeline(pApp);
}

void mainLoop(App *pApp) {
  while (!glfwWindowShouldClose(pApp->window)) {
    glfwPollEvents();
  }
}

void cleanup(App *pApp) {
  vkDestroyPipeline(pApp->device, pApp->graphicsPipeline, NULL);
  vkDestroyPipelineLayout(pApp->device, pApp->pipelineLayout, NULL);
  vkDestroyRenderPass(pApp->device, pApp->renderPass, NULL);

  for (u32 i = 0; i < pApp->swapChainImageCount; i++) {
    vkDestroyImageView(pApp->device, pApp->swapChainImageViews[i], NULL);
  }

  vkDestroySwapchainKHR(pApp->device, pApp->swapChain, NULL);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(pApp->instance, pApp->debugMessenger, NULL);
  }
  vkDestroyDevice(pApp->device, NULL);

  vkDestroySurfaceKHR(pApp->instance, pApp->surface, NULL);
  vkDestroyInstance(pApp->instance, NULL);

  glfwDestroyWindow(pApp->window);

  glfwTerminate();
}

bool verfityExtensionSupport(
  u32 extensionCount,
  VkExtensionProperties *extensions,
  u32 glfwExtensionCount,
  const char** glfwExtensions) {

  for (u32 i = 0; i < glfwExtensionCount; i++) {
    bool extensionFound = false;
    for (u32 j = 0; j < extensionCount; j++) {
      if (strcmp(extensions[j].extensionName, glfwExtensions[i]) == 0) {
        extensionFound = true;
        break;
      }
    }
    if (!extensionFound) {
      return false;
    }
  }

  return true;
}

void createInstance(App *pApp) {
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    printf("Validation layers requested but not available!\n");
    exit(1);
  }

  VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = WIN_TITLE,
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "No Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_0,
    .pNext = NULL
  };

  u32 glfwExtensionCount = 0;
  const char** glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  const char *glfwExtensionsWithDebug[glfwExtensionCount+1];

  for (u32 i = 0; i < glfwExtensionCount; i++) {
    glfwExtensionsWithDebug[i] = glfwExtensions[i];
  }
  if (enableValidationLayers) {
    glfwExtensionsWithDebug[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  }

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
  VkInstanceCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo
  };
  if (enableValidationLayers) {
    createInfo.enabledLayerCount = validationLayerCount;
    createInfo.ppEnabledLayerNames = validationLayers;
    createInfo.enabledExtensionCount = glfwExtensionCount + 1;
    createInfo.ppEnabledExtensionNames = glfwExtensionsWithDebug;

    populateDebugMessengerCreateInfo(&debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.pNext = NULL;
  }

  if (vkCreateInstance(&createInfo, NULL, &pApp->instance) != VK_SUCCESS) {
    printf("Failed to create Vulkan Instance\n");
    exit(1);
  }

  // Vulkan extensions
  u32 extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

  VkExtensionProperties extensions[extensionCount];
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

  if (!verfityExtensionSupport(
        extensionCount,
        extensions,
        glfwExtensionCount,
        glfwExtensions)) {
    printf("Missing extension support!\n");
    exit(1);
  }
}

void createSurface(App *pApp) {
  if (glfwCreateWindowSurface(pApp->instance, pApp->window, NULL, &pApp->surface) != VK_SUCCESS) {
    printf("Failed to create window surface!\n");
    exit(5);
  }
}

void pickPhysicalDevice(App *pApp) {
  u32 deviceCount = 0;
  vkEnumeratePhysicalDevices(pApp->instance, &deviceCount, NULL);

  if (deviceCount == 0) {
    printf("Failed to find a GPU with Vulkan support!\n");
    exit(3);
  }

  VkPhysicalDevice devices[deviceCount];
  vkEnumeratePhysicalDevices(pApp->instance, &deviceCount, devices);

  VkPhysicalDevice device;
  u32 deviceScore = 0;
  for (u32 i = 0; i < deviceCount; i++) {
    u32 score = rateDeviceSuitability(devices[i], pApp->surface);
    if (score > deviceScore) {
      deviceScore = score;
      device = devices[i];
    }
  }

  if (device == NULL) {
    printf("Failed to find a stuitable GPU!\n");
    exit(3);
  }
  pApp->physicalDevice = device;
  printf("GPU selected\n");

  pApp->queueFamilyIndices = findQueueFamilies(device, pApp->surface);
}

void getFamilyDeviceQueues(VkDeviceQueueCreateInfo *queues, QueueFamilyIndices indices, VkPhysicalDeviceFeatures deviceFeatures) {
  // GraphicsQueue
  VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = indices.graphicsFamily,
    .queueCount = 1
  };

  float graphicsQueuePriority = 1.0f;
  graphicsQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;
  queues[0] = graphicsQueueCreateInfo;

  // PresentQueue
  VkDeviceQueueCreateInfo presentQueueCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = indices.presentFamily,
    .queueCount = 1
  };

  float presentQueuePriority = 1.0f;
  graphicsQueueCreateInfo.pQueuePriorities = &presentQueuePriority;
  queues[1] = presentQueueCreateInfo;
}

void createLogicalDevice(App *pApp) {
  QueueFamilyIndices indices = findQueueFamilies(pApp->physicalDevice, pApp->surface);

  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(pApp->physicalDevice, &deviceFeatures);

  VkDeviceQueueCreateInfo queues[2];
  getFamilyDeviceQueues(queues, indices, deviceFeatures);

  VkDeviceCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    //.pQueueCreateInfos = &queueCreateInfo,
    .pQueueCreateInfos = queues,
    .queueCreateInfoCount = 1,
    .pEnabledFeatures = &deviceFeatures,
    .enabledExtensionCount = deviceExtensionCount,
    .ppEnabledExtensionNames = deviceExtensions
  };

  if (enableValidationLayers) {
    createInfo.enabledLayerCount = validationLayerCount;
    createInfo.ppEnabledLayerNames = validationLayers;
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(pApp->physicalDevice, &createInfo, NULL, &pApp->device) != VK_SUCCESS) {
    printf("Failed to create logical device!\n");
    exit(4);
  }

  vkGetDeviceQueue(pApp->device, pApp->queueFamilyIndices.graphicsFamily, 0, &pApp->graphicsQueue);
}

void createSwapChain(App *pApp) {
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(pApp->physicalDevice, pApp->surface);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formatCount, swapChainSupport.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModeCount, swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(pApp->window, swapChainSupport.capabilities);

  u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = pApp->surface,
    .minImageCount = imageCount,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
  };

  QueueFamilyIndices indices = findQueueFamilies(pApp->physicalDevice, pApp->surface);
  u32 queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = NULL; // Optional
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(pApp->device, &createInfo, NULL, &pApp->swapChain) != VK_SUCCESS) {
    printf("Failed to create Swap Chain!\n");
    exit(5);
  }

  vkGetSwapchainImagesKHR(pApp->device, pApp->swapChain, &imageCount, NULL);
  pApp->swapChainImages = (VkImage*)malloc(sizeof(VkImage) * imageCount);

  vkGetSwapchainImagesKHR(pApp->device, pApp->swapChain, &imageCount, pApp->swapChainImages);
  pApp->swapChainImageCount = imageCount;

  pApp->swapChainImageFormat = surfaceFormat.format;
  pApp->swapChainExtent = extent;
}

void createImageViews(App *pApp) {
  pApp->swapChainImageViews = (VkImageView*)malloc(sizeof(VkImageView) * pApp->swapChainImageCount);

  for (u32 i = 0; i < pApp->swapChainImageCount; i++) {
    VkImageViewCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = pApp->swapChainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = pApp->swapChainImageFormat,
      .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
      .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1
    };

    if (vkCreateImageView(pApp->device, &createInfo, NULL, &pApp->swapChainImageViews[i]) != VK_SUCCESS) {
      printf("Failed to create image views!\n");
      exit(6);
    }
  }
}

typedef struct ShaderFile {
  size_t size;
  char *code;
} ShaderFile;

VkShaderModule createShaderModule(App *pApp, ShaderFile *shaderFile) {
  VkShaderModuleCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = shaderFile->size,
    .pCode = (u32*)shaderFile->code
  };

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(pApp->device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
    printf("failed to create shader module!\n");
    exit(7);
  }

  return shaderModule;
}

void readFile(const char *filename, ShaderFile *shader) {
  FILE *pFile;

  pFile = fopen(filename, "rb");
  if (pFile == NULL) {
    printf("Failed to open %s\n", filename);
    exit(7);
  }

  fseek(pFile, 0L, SEEK_END);
  shader->size = ftell(pFile);

  fseek(pFile, 0L, SEEK_SET);

  shader->code = (char*)malloc(sizeof(char) * shader->size);
  size_t readCount = fread(shader->code, shader->size, sizeof(char), pFile);
  printf("ReadCount: %ld\n", readCount);

  fclose(pFile);
}

void createRenderPass(App *pApp) {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = pApp->swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  if (vkCreateRenderPass(pApp->device, &renderPassInfo, NULL, &pApp->renderPass) != VK_SUCCESS) {
    printf("failed to create render pass!\n");
    exit(8);
  }
}

void createGraphicsPipeline(App *pApp) {
  ShaderFile vertShader = {0};
  ShaderFile fragShader = {0};
  readFile("shaders/vert.spv", &vertShader);
  readFile("shaders/frag.spv", &fragShader);

  VkShaderModule vertShaderModule = createShaderModule(pApp, &vertShader);
  VkShaderModule fragShaderModule = createShaderModule(pApp, &fragShader);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertShaderModule,
    .pName = "main"
    //.pSpecializationInfo = NULL
  };

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragShaderModule,
    .pName = "main"
  };

  VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 0,
    .pVertexBindingDescriptions = NULL, // Optional
    .vertexAttributeDescriptionCount = 0,
    .pVertexAttributeDescriptions = NULL // Optional
  };

  u32 dynamicStatesSize = 2;
  VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

  VkPipelineDynamicStateCreateInfo dynamicState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = dynamicStatesSize,
    .pDynamicStates = dynamicStates
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  VkViewport viewport = {
    .x = 0.0f,
    .y = 0.0f,
    .width = (float)pApp->swapChainExtent.width,
    .height = (float)pApp->swapChainExtent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };

  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = pApp->swapChainExtent
  };

  VkPipelineViewportStateCreateInfo viewportState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor
  };

  VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .lineWidth = 1.0f,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0f, // Optional
    .depthBiasClamp = 0.0f, // Optional
    .depthBiasSlopeFactor = 0.0f // Optional
  };

  VkPipelineMultisampleStateCreateInfo multisampling = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .minSampleShading = 1.0f, // Optional
    .pSampleMask = NULL, // Optional
    .alphaToCoverageEnable = VK_FALSE, // Optional
    .alphaToOneEnable = VK_FALSE // Optional
  };

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .colorBlendOp = VK_BLEND_OP_ADD, // Optional
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
    .alphaBlendOp = VK_BLEND_OP_ADD // Optional
  };

  VkPipelineColorBlendStateCreateInfo colorBlending = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY, // Optional
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants[0] = 0.0f, // Optional
    .blendConstants[1] = 0.0f, // Optional
    .blendConstants[2] = 0.0f, // Optional
    .blendConstants[3] = 0.0f // Optional
  };

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 0, // Optional
    .pSetLayouts = NULL, // Optional
    .pushConstantRangeCount = 0, // Optional
    .pPushConstantRanges = NULL // Optional
  };

  if (vkCreatePipelineLayout(pApp->device, &pipelineLayoutInfo, NULL, &pApp->pipelineLayout) != VK_SUCCESS) {
    printf("failed to create pipeline layout!");
    exit(7);
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.flags = 0;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = NULL; // Optional
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pApp->pipelineLayout;
  pipelineInfo.renderPass = pApp->renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
  pipelineInfo.basePipelineIndex = -1; // Optional

  if (vkCreateGraphicsPipelines(pApp->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pApp->graphicsPipeline) != VK_SUCCESS) {
    printf("Failed to create graphics pipeline!\n");
    exit(9);
  }

  free(vertShader.code);
  free(fragShader.code);
  vkDestroyShaderModule(pApp->device, fragShaderModule, NULL);
  vkDestroyShaderModule(pApp->device, vertShaderModule, NULL);
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
  u32 extensionCount;
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);
  VkExtensionProperties availableExtensions[extensionCount];
  vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

  for (u32 i = 0; i < deviceExtensionCount; i++) {
    bool extensionFound = false;
    for (u32 j = 0; j < extensionCount; j++) {
      if (strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0) {
        extensionFound = true;
        break;
      }
    }
    if (!extensionFound) {
      return false;
    }
  }

  return true;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  u32 formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
  details.formatCount = formatCount;
  VkSurfaceFormatKHR formats[formatCount];
  details.formats = formats;
  if (formatCount != 0) {
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
  }

  u32 presentCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, NULL);
  details.presentModeCount = presentCount;
  VkPresentModeKHR presentModes[presentCount];
  details.presentModes = presentModes;
  if (presentCount != 0) {
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentCount, details.presentModes);
  }

  return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(u32 formatCount, VkSurfaceFormatKHR *availableFormats) {
  for (u32 i = 0; i < formatCount; i++) {
    if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormats[i];
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(u32 presentModeCount, VkPresentModeKHR *availablePresentModes) {
  for (u32 i = 0; i < presentModeCount; i++) {
    if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentModes[i];
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capabilities) {
  if (capabilities.currentExtent.width != UINT_MAX) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = { (u32)width, (u32)height };

    actualExtent.width = clamp_u32(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = clamp_u32(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

u32 rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface) {
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  u32 score = 0;

  // Discrete GPUs have a significant performance advantage
  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    score += 1000;
  }

  // Maximum possible size of textures affects graphics quality
  score += deviceProperties.limits.maxImageDimension2D;

  // Applications can't function without geometry shaders
  if (!deviceFeatures.geometryShader) {
    return 0;
  }

  // Check device supports required queue families
  // Note: to improve performance, we could favour queue families that have both graphcs and present support.
  // We could check the returned indices and if they are the same, increase the score.
  QueueFamilyIndices indices = findQueueFamilies(device, surface);
  if (!indices.isGraphicsFamilySet) {
    printf("Queue Family not supported!\n");
    return 0;
  }

  bool extensionsSupported = checkDeviceExtensionSupport(device);
  if (!extensionsSupported) {
    printf("Required device extensions not supported!\n");
    return 0;
  }

  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
  if (swapChainSupport.formatCount == 0 || swapChainSupport.presentModeCount == 0) {
    printf("Swap chain not adequately supported!\n");
    return 0;
  }

  return score;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
  QueueFamilyIndices indices = {0};

  u32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

  VkQueueFamilyProperties queueFamilyProperties[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties);

  for (u32 i = 0; i < queueFamilyCount; i++) {
    if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
      indices.isGraphicsFamilySet = true;
      break;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
      indices.isPresentFamilySet = true;
    }
  }


  return indices;
}

bool checkValidationLayerSupport() {
  u32 layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, NULL);

  VkLayerProperties availableLayers[layerCount];
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

  for (u32 i = 0; i < validationLayerCount; i++) {
    bool layerFound = false;
    for (u32 j = 0; j < layerCount; j++) {
      if (strcmp(availableLayers[j].layerName, validationLayers[i]) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }

  return true;
}

void setupDebugMessenger(App *pApp) {
  if (!enableValidationLayers) return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
  populateDebugMessengerCreateInfo(&createInfo);
  // createInfo.pUserData = NULL;

  if (CreateDebugUtilsMessengerEXT(pApp->instance, &createInfo, NULL, &pApp->debugMessenger) != VK_SUCCESS) {
    printf("Failed to setup debug messenger!\n");
    exit(2);
  }
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *createInfo) {
  createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo->pfnUserCallback = debugCallback;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

  printf("validation layer: %s\n", pCallbackData->pMessage);

  return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

  if (func != NULL) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

u32 clamp_u32(u32 n, u32 min, u32 max) {
  if (n < min) return min;
  if (n > max) return max;
  return n;
}
