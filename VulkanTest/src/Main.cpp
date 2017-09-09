#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <cstring>

const int WIDTH = 800;
const int HEIGHT = 600;


const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	std::cerr << "validation layer msg : " << msg << std::endl;

	return VK_FALSE;
}

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

bool checkValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
			return false;
	}

	return true;
}

std::vector<const char*> getRequiredExtensions()
{
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
	{
		extensions.push_back(glfwExtensions[i]);
	}

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

class HelloTriangleApplication 
{
private:
	GLFWwindow* window;
	VkInstance instance;
	VkDebugReportCallbackEXT callback;

public:

	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:

	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

	void createInstance();
	void setupDebugCallback();

};

void HelloTriangleApplication::setupDebugCallback()
{
	if (!enableValidationLayers)
		return;

	VkDebugReportCallbackCreateInfoEXT createInfos = {};
	createInfos.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfos.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfos.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfos, nullptr, &callback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug callback !");
	}
}

void HelloTriangleApplication::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(600, 600, "VulkanTest", nullptr, nullptr);

	if (window == nullptr) {
		throw std::runtime_error("failed to create window !");
	}
}

void HelloTriangleApplication::initVulkan()
{
	createInstance();
	setupDebugCallback();
}

void HelloTriangleApplication::createInstance()
{
	if (enableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requestd, but not available !");
	}

	VkApplicationInfo appInfos;
	appInfos.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfos.pApplicationName = "VulkanTest";
	appInfos.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfos.pEngineName = "No engine";
	appInfos.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfos.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceInfos;
	instanceInfos.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfos.pApplicationInfo = &appInfos;

	auto extensions = getRequiredExtensions();
	instanceInfos.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceInfos.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers) {
		instanceInfos.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceInfos.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		instanceInfos.enabledLayerCount = 0;
	}

	instanceInfos.flags = 0;
	instanceInfos.pNext = nullptr;

	if (vkCreateInstance(&instanceInfos, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance !");
	}

	// print extensions' name
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
		std::cout << "availlable extensions : " << std::endl;
		for (const auto& extension : extensions) {
			std::cout << "\t" << extension.extensionName << std::endl;
		}
	}
}

void HelloTriangleApplication::mainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

void HelloTriangleApplication::cleanup()
{
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}

int main()
{
	HelloTriangleApplication app;

	try {
		app.run();

	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}