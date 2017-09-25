#include "VulkanUtils.h"
#include "Buffer.h"
#include "Image.h"
#include <fstream>
#include <set>

VkCommandBuffer beginSingleTimeTransferCommands(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBuffer commandBuffer;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void endSingleTimeTransferCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue transferQueue)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

uint32_t getMemoryTypeIndexFromMemoryTypeBit(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (size_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i)
			&& memProperties.memoryTypes[i].propertyFlags & properties)
			return i;
	}

	throw std::runtime_error("failed to fin suitable memory type !");
}

void cmdCopyBufferToBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkQueue transferQueue, Buffer& from, Buffer& to, const BufferCopyInfo* copyInfo, uint32_t copyInfoCount)
{
	std::vector<VkBufferCopy> copyRegionInfos(copyInfoCount);
	for (size_t i = 0; i < copyInfoCount; i++)
	{
		copyRegionInfos[i].srcOffset = copyInfo->srcOffset;
		copyRegionInfos[i].dstOffset = copyInfo->dstOffset;
		copyRegionInfos[i].size = copyInfo->size;
	}
	vkCmdCopyBuffer(commandBuffer, *from.getBufferHandle(), *to.getBufferHandle(), copyInfoCount, copyRegionInfos.data());
}

void singleCmdCopyBufferToBuffer(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue, Buffer& from, Buffer& to, const BufferCopyInfo* copyInfo, uint32_t copyInfoCount)
{
	VkCommandBuffer commandBuffer = beginSingleTimeTransferCommands(device, commandPool);

	cmdCopyBufferToBuffer(device, commandBuffer, transferQueue, from, to, copyInfo, copyInfoCount);

	endSingleTimeTransferCommands(device, commandPool, commandBuffer, transferQueue);
}

void cmdCopyBufferToImage(VkDevice device, VkCommandBuffer commandBuffer, VkQueue transferQueue, Buffer& from, Image2D& to)
{
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { to.getWidth(), to.getHeight(), 1 };

	vkCmdCopyBufferToImage(commandBuffer, *from.getBufferHandle(), to.getImageHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void singleCmdCopyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue, Buffer& from, Image2D& to)
{
	VkCommandBuffer commandBuffer = beginSingleTimeTransferCommands(device, commandPool);

	cmdCopyBufferToImage(device, commandBuffer, transferQueue, from, to);

	endSingleTimeTransferCommands(device, commandPool, commandBuffer, transferQueue);
}

void cmdTransitionImageLayout(VkDevice device, VkCommandBuffer commandBuffer, VkQueue transferQueue
	, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, const TransitionAccessInfo& accessInfo)
{
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.srcAccessMask = accessInfo.srcAccessMask;
	barrier.dstAccessMask = accessInfo.dstAccessMask;

	vkCmdPipelineBarrier(commandBuffer, accessInfo.srcStageFlag, accessInfo.dstStageFlag, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
void singleCmdTransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue
	, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, const TransitionAccessInfo& accessInfo)
{
	VkCommandBuffer commandBuffer = beginSingleTimeTransferCommands(device, commandPool);

	cmdTransitionImageLayout(device, commandBuffer, transferQueue, image, oldLayout, newLayout, accessInfo);

	endSingleTimeTransferCommands(device, commandPool, commandBuffer, transferQueue);
}

void cmdTransitionImageLayout(VkDevice device, VkCommandBuffer commandBuffer, VkQueue transferQueue
	, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	TransitionAccessInfo accessInfo = findTransitionAccessInfo(oldLayout, newLayout);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.srcAccessMask = accessInfo.srcAccessMask;
	barrier.dstAccessMask = accessInfo.dstAccessMask;

	vkCmdPipelineBarrier(commandBuffer, accessInfo.srcStageFlag, accessInfo.dstStageFlag, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
void singleCmdTransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue
	, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeTransferCommands(device, commandPool);

	cmdTransitionImageLayout(device, commandBuffer, transferQueue, image, oldLayout, newLayout);

	endSingleTimeTransferCommands(device, commandPool, commandBuffer, transferQueue);
}

TransitionAccessInfo findTransitionAccessInfo(VkImageLayout oldLayout, VkImageLayout newLayout)
{
	TransitionAccessInfo outAccessInfo;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
		&& newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		outAccessInfo.srcAccessMask = 0;
		outAccessInfo.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		outAccessInfo.srcStageFlag = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		outAccessInfo.dstStageFlag = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		&& newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		outAccessInfo.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		outAccessInfo.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		outAccessInfo.srcStageFlag = VK_PIPELINE_STAGE_TRANSFER_BIT;
		outAccessInfo.dstStageFlag = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
		&& newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		outAccessInfo.srcAccessMask = 0;
		outAccessInfo.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		outAccessInfo.srcStageFlag = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		outAccessInfo.dstStageFlag = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition !");
	}

	return outAccessInfo;
}

bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat findSupportedFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR 
			&& ((props.linearTilingFeatures & features) == features))
			return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL 
			&& ((props.optimalTilingFeatures & features) == features))
			return format;
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat findDepthFormat(VkPhysicalDevice& physicalDevice, size_t* outFormatSize, uint16_t* outFormatComponentCount)
{
	VkFormat format = findSupportedFormat(physicalDevice,
	{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }
		, VK_IMAGE_TILING_OPTIMAL
		, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	switch (format)
	{
	case VK_FORMAT_D32_SFLOAT:
		if(outFormatSize != nullptr)
			*outFormatSize = 4;
		if(outFormatComponentCount != nullptr)
			*outFormatComponentCount = 1;
		break;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		if (outFormatSize != nullptr)
			*outFormatSize = 5;
		if (outFormatComponentCount != nullptr)
			*outFormatComponentCount = 2;
		break;
	case VK_FORMAT_D24_UNORM_S8_UINT:
		if (outFormatSize != nullptr)
			*outFormatSize = 4;
		if (outFormatComponentCount != nullptr)
			*outFormatComponentCount = 2;
		break;
	}

	return format;
}

VkFormat findDepthAndStencilFormat(VkPhysicalDevice& physicalDevice, size_t* outFormatSize, uint16_t* outFormatComponentCount)
{
	VkFormat format = findSupportedFormat(physicalDevice,
	{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }
		, VK_IMAGE_TILING_OPTIMAL
		, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	switch (format)
	{
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		if (outFormatSize != nullptr)
			*outFormatSize = 5;
		if (outFormatComponentCount != nullptr)
			*outFormatComponentCount = 2;
		break;
	case VK_FORMAT_D24_UNORM_S8_UINT:
		if (outFormatSize != nullptr)
			*outFormatSize = 4;
		if (outFormatComponentCount != nullptr)
			*outFormatComponentCount = 2;
		break;
	}

	return format;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<const char*>& deviceExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool checkSwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	if (formatCount == 0)
		return false;

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount == 0)
		return false;

	return true;
}

int ratePhysicalDeviceSuitability(VkPhysicalDevice physicalDevice
	, VkSurfaceKHR surface
	, const std::vector<const char*>& requiredDeviceExtensions
	, const VkPhysicalDeviceFeatures& requiredDeviceFeatures
	, bool needPresentSupport
	, VkQueueFlags requestedQueueFlags)
{
	int score = 0;

	// check present support if needed and check we have all queue flags in queues inside this device
	VkQueueFlags foundQueueFlags = requestedQueueFlags;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());
	VkBool32 presentSupport = false;
	for (int queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
	{
		if (queueFamilyProps[queueFamilyIndex].queueCount > 0)
		{
			if(!presentSupport)
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &presentSupport);

			if (queueFamilyProps[queueFamilyIndex].queueFlags & requestedQueueFlags)
				foundQueueFlags = foundQueueFlags & ~requestedQueueFlags;
		}


	}
	if ( (needPresentSupport && !presentSupport) || (foundQueueFlags != 0))
		return 0;

	// Find extensions we want
	bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice, requiredDeviceExtensions);
	// Check we found an adequate swap chain support
	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		swapChainAdequate = checkSwapChainSupport(physicalDevice, surface);
	}
	if (!extensionsSupported || !swapChainAdequate)
		return 0;


	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	if (!deviceFeatures.geometryShader || !deviceFeatures.samplerAnisotropy)
		return 0;

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	score += deviceProperties.limits.maxImageDimension2D;

	return score;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentMode.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentMode.data());
	}

	return details;
}

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module !");
	}

	return shaderModule;
}

std::vector<char> readShaderFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file ! ");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
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

bool checkValidationLayerSupport(const std::vector<const char*>& requestedValidationLayers)
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : requestedValidationLayers)
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
