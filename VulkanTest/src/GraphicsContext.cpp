#include "GraphicsContext.h"

#include <GLFW/glfw3.h>

#include "Renderer.h"
#include "VulkanUtils.h"

void GraphicsContext::createInstance(const RenderSetup& renderSetup)
{
	if (renderSetup.validationLayersEnabled && !checkValidationLayerSupport(renderSetup.validationLayers))
	{
		throw std::runtime_error("validation layers requestd, but not available !");
	}

	VkApplicationInfo appInfos;
	appInfos.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfos.pApplicationName = "Volcano";
	appInfos.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfos.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfos.pEngineName = "Volcano";
	appInfos.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfos;
	const auto& extensions = renderSetup.deviceExtensions;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	if (renderSetup.validationLayersEnabled)
	{
		const auto& validationLayers = renderSetup.validationLayers;
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}
	createInfo.flags = 0;
	createInfo.pNext = nullptr;

	CHECK_VK_THROW_ERROR(vkCreateInstance(&createInfo, nullptr, &instance), "failed to create instance !");
}

void GraphicsContext::setupDebugCallback(const RenderSetup& renderSetup)
{
	if (!renderSetup.validationLayersEnabled)
		return;

	VkDebugReportCallbackCreateInfoEXT createInfos = {};
	createInfos.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfos.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfos.pfnCallback = vulkanDebugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfos, nullptr, &callback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug callback !");
	}
}

void GraphicsContext::createPhysicalDevice(const RenderSetup& renderSetup, VkSurfaceKHR surface)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
	CHECK_TRUE_THROW_ERROR(physicalDeviceCount == 0, "failed to find GPUs with Vulkan support !");

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

	std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& physicalDevice : physicalDevices)
	{
		int score = ratePhysicalDeviceSuitability(physicalDevice, surface
													, renderSetup.deviceExtensions
													, renderSetup.requiredDeviceFeatures
													, renderSetup.needPresentSupport
													, renderSetup.requestedQueueFlags);
		candidates.insert(std::make_pair(score, device));
	}

	if (candidates.rbegin()->first > 0)
		physicalDevice = candidates.rbegin()->second;
	else
		throw std::runtime_error("failed to find a suitable GPU!");

	// We store the properties for later use
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
}

void GraphicsContext::initQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());

	int i = 0;
	for (const auto& queueFamilyProp : queueFamilyProps)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

		if (queueFamilyProp.queueCount > 0
			&& presentSupport)
			queueFamilies.presentFamily = i;

		if (queueFamilyProp.queueCount > 0
			&& (queueFamilyProp.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			queueFamilies.graphicFamily = i;

		if (queueFamilies.isComplete())
			break;

		i++;
	}

	CHECK_THROW_ERROR(queueFamilies.isComplete(), "failed to find required queue families !");
}

void GraphicsContext::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilies.graphicFamily;
	poolInfo.flags = 0; //Optional

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool !");
	}
}

void GraphicsContext::createDevice(const RenderSetup& renderSetup) 
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = {};
	std::set<int> uniqueQueueFamilies = { queueFamilies.graphicFamily, queueFamilies.presentFamily };

	float queuePriorities = 1.0f;
	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriorities;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	const VkPhysicalDeviceFeatures& deviceFeatures = renderSetup.requiredDeviceFeatures;
	const std::vector<const char*>& deviceExtensions = renderSetup.deviceExtensions;
	bool enableValidationLayers = renderSetup.validationLayersEnabled;
	const std::vector<const char*>& validationLayers = renderSetup.validationLayers;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device !");
	}

	// get graphics queue from device
	vkGetDeviceQueue(device, queueFamilies.graphicFamily, 0, &graphicsQueue);
	// get present queue from device
	vkGetDeviceQueue(device, queueFamilies.presentFamily, 0, &presentQueue);
}

void GraphicsContext::destroy()
{
	vkDestroyDevice(device, nullptr);
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);

	vkDestroyInstance(instance, nullptr);

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	QueueFamilies queueFamilies;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkDebugReportCallbackEXT callback;
}

VkInstance GraphicsContext::getInstance() const
{
	return instance;
}

VkDevice GraphicsContext::getDevice() const
{
	return device;
}

VkPhysicalDevice GraphicsContext::getPhysicalDevice() const
{
	return physicalDevice;
}

VkCommandPool GraphicsContext::getCommandPool() const
{
	return commandPool;
}

//////////////////////////////////////////////

void WindowContext::createSurface(VkInstance instance, GLFWwindow& window)
{
	CHECK_VK_THROW_ERROR(glfwCreateWindowSurface(instance, &window, nullptr, &surface), "failed to create window surface !");
}

void WindowContext::createSwapChain(const glm::vec2& windowSize, VkPhysicalDevice physicalDevice, VkDevice device, GraphicsContext::QueueFamilies queueFamilies)
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentMode);
	VkExtent2D extent = chooseSwapExtent(windowSize, swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0
		&& imageCount > swapChainSupport.capabilities.maxImageCount)
		imageCount = swapChainSupport.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { (uint32_t)queueFamilies.graphicFamily, (uint32_t)queueFamilies.presentFamily };
	if (queueFamilies.graphicFamily != queueFamilies.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain !");
	}

	// Get images
	{
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	// Create image views
	{
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views !");
			}
		}
	}
}

void WindowContext::cleanupSwapChain()
{
	//TODO
}

void WindowContext::destroy(VkInstance instance)
{
	cleanupSwapChain();
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

VkSurfaceFormatKHR WindowContext::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	}

	return availableFormats[0];
}

VkPresentModeKHR WindowContext::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			bestMode = availablePresentMode;
	}

	return bestMode;
}

VkExtent2D WindowContext::chooseSwapExtent(const glm::vec2& windowSize, const VkSurfaceCapabilitiesKHR& capabilities)
{
	VkExtent2D actualExtent = { windowSize.x, windowSize.y};

	actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

	return actualExtent;
}

VkSurfaceKHR WindowContext::getSurface() const
{
	return surface;
}

VkSwapchainKHR WindowContext::getSwapChain() const
{
	return swapChain;
}

uint32_t WindowContext::getImageCount() const
{
	return swapChainImages.size();
}

VkImage WindowContext::getImage(uint32_t imageIndex) const
{
	return swapChainImages[imageIndex];
}

VkImageView WindowContext::getImageView(uint32_t imageIndex) const
{
	return swapChainImageViews[imageIndex];
}
