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

typedef struct App {
  GLFWwindow *window;
  VkInstance instance;
} App;

void initWindow(App *pApp);
void initVulkan(App *pApp);
void mainLoop(App *pApp);
void cleanup(App *pApp);

void createInstance(App *pApp);

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
}

void mainLoop(App *pApp) {
  while (!glfwWindowShouldClose(pApp->window)) {
    glfwPollEvents();
  }
}

void cleanup(App *pApp) {
  vkDestroyInstance(pApp->instance, NULL);

  glfwDestroyWindow(pApp->window);

  glfwTerminate();
}

bool verfityExtensionSupport(
  u32 extensionCount,
  VkExtensionProperties *extensions,
  u32 glfwExtensionCount,
  const char** glfwExtensions) {
  u32 foundExtensions = 0;
  for (int i = 0; i < glfwExtensionCount; i++) {
    for (int j = 0; j < extensionCount; j++) {
      if (strcmp(extensions[j].extensionName, glfwExtensions[i]) == 0) {
        foundExtensions++;
      }
    }
  }

  return foundExtensions == glfwExtensionCount;
}

void createInstance(App *pApp) {
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

  VkInstanceCreateInfo createInfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo,
    .enabledExtensionCount = glfwExtensionCount,
    .ppEnabledExtensionNames = glfwExtensions
  };
  createInfo.enabledLayerCount = 0;

  if (vkCreateInstance(&createInfo, NULL, &pApp->instance) != VK_SUCCESS) {
    printf("Failed to create Vulkan Instance\n");
    exit(1);
  }

  u32 extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

  VkExtensionProperties *extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * extensionCount);
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
