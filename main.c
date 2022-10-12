#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const char* WIN_TITLE = "SeEngine";
const uint32_t WIN_WIDTH = 800;
const uint32_t WIN_HEIGHT = 600;

typedef struct App {
  GLFWwindow *window;
} App;

void initWindow(App *pApp);
void initVulkan(void);
void mainLoop(App *pApp);
void cleanup(App *pApp);

int main(void) {
  App app = {0};

  initWindow(&app);
  initVulkan();
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

void initVulkan(void) {}

void mainLoop(App *pApp) {
  while (!glfwWindowShouldClose(pApp->window)) {
    glfwPollEvents();
  }
}

void cleanup(App *pApp) {
  glfwDestroyWindow(pApp->window);

  glfwTerminate();
}
