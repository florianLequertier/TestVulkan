#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>

class WindowHandler
{
private:
	GLFWwindow* window;

public:
	std::function<void(float, float)> windowResizeCallback;

	void create(const glm::vec2& windowSize = glm::vec2(800, 600), const std::string& windowTitle = "Title");
	void destroy();

	void createWindow(const glm::vec2& initialSize, const std::string& title);
	void destroyWindow();

	static void onWindowResized(GLFWwindow* window, int width, int height);
	std::vector<const char*> getRequiredWindowExtensions() const;
	GLFWwindow* getWindow() const;
};