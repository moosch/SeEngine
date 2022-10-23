#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
}

void mainLoop(App *pApp) {
  while (!glfwWindowShouldClose(pApp->window)) {
    glfwPollEvents();
  }
}

void cleanup(App *pApp) {
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

  for (int i = 0; i < glfwExtensionCount; i++) {
    bool extensionFound = false;
    for (int j = 0; j < extensionCount; j++) {
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

  for (int i = 0; i < glfwExtensionCount; i++) {
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
  if (device == NULL) printf("Device starts as NULL\n");

  u32 deviceScore = 0;
  for (int i = 0; i < deviceCount; i++) {
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

  return score;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
  QueueFamilyIndices indices = {0};

  u32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

  VkQueueFamilyProperties queueFamilyProperties[queueFamilyCount];
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties);

  for (int i = 0; i < queueFamilyCount; i++) {
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

  for (int i = 0; i < validationLayerCount; i++) {
    bool layerFound = false;
    for (int j = 0; j < layerCount; j++) {
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
