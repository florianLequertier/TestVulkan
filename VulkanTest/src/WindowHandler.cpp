#include "WindowHandler.h"

void WindowHandler::create(const glm::vec2& windowSize, const std::string& windowTitle)
{
	createWindow(windowSize, windowTitle);
}

void WindowHandler::destroy()
{
	destroyWindow();
}

void WindowHandler::createWindow(const glm::vec2& initialSize, const std::string& title)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(600, 600, "VulkanTest", nullptr, nullptr);

	if (window == nullptr)
		throw std::runtime_error("failed to create window !");

	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, WindowHandler::onWindowResized);
}

void WindowHandler::destroyWindow()
{
	glfwDestroyWindow(window);
}

void WindowHandler::onWindowResized(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0) return;

	WindowHandler* handler = reinterpret_cast<WindowHandler*>(glfwGetWindowUserPointer(window));
	if (handler->windowResizeCallback)
		handler->windowResizeCallback(width, height);
}

std::vector<const char*> WindowHandler::getRequiredWindowExtensions() const
{
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
	{
		extensions.push_back(glfwExtensions[i]);
	}

	return extensions;
}

GLFWwindow* WindowHandler::getWindow() const
{
	return window;
}