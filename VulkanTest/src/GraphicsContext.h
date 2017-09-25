#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

class Renderer;
struct RenderSetup;
class GLFWwindow;

class GraphicsContext
{
public:
	struct QueueFamilies
	{
		int graphicFamily = -1;
		int presentFamily = -1;

		bool isComplete() {
			return graphicFamily >= 0 && presentFamily >= 0;
		}
	};

private:
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkDevice device;
	QueueFamilies queueFamilies;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkDebugReportCallbackEXT callback;
	VkCommandPool commandPool;

public:
	void createInstance(const RenderSetup& renderSetup);
	void setupDebugCallback(const RenderSetup& renderSetup);
	void createPhysicalDevice(const RenderSetup& renderSetup, VkSurfaceKHR surface);
	void createDevice(const RenderSetup& renderSetup);
	void initQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	void createCommandPool();
	void destroy();

	VkInstance getInstance() const;
	VkDevice getDevice() const;
	VkPhysicalDevice getPhysicalDevice() const;
	VkCommandPool getCommandPool() const;

	inline const QueueFamilies& getQueueFamilies() const
	{
		return queueFamilies;
	}
	inline VkQueue getGraphicsQueue() const
	{
		return graphicsQueue;
	}
	inline VkQueue getPresentQueue() const
	{
		return presentQueue;
	}
	inline VkPhysicalDeviceProperties getPhysicalDeviceProperties() const
	{
		return physicalDeviceProperties;
	}
	inline uint32_t getUBOAlignement()
	{
		getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	}
};

class WindowContext
{

private:
	VkSurfaceKHR surface;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

public:
	void createSurface(VkInstance instance, GLFWwindow& window);
	void createSwapChain(const glm::vec2& windowSize, VkPhysicalDevice physicalDevice, VkDevice device, GraphicsContext::QueueFamilies queueFamilies);
	
	void cleanupSwapChain();
	void destroy(VkInstance instance);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const glm::vec2& windowSize, const VkSurfaceCapabilitiesKHR& capabilities);

	VkSurfaceKHR getSurface() const;
	VkSwapchainKHR getSwapChain() const;
	uint32_t getImageCount() const;
	VkImage getImage(uint32_t imageIndex) const;
	VkImageView getImageView(uint32_t imageIndex) const;
};