#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void process_input(GLFWwindow *window);

#endif