#pragma once

#include <vulkan/vulkan.hpp>
#include <iostream>

class Buffer;
class Image2D;
struct BufferCopyInfo;

struct TransitionAccessInfo
{
	VkPipelineStageFlags srcStageFlag;
	VkAccessFlags srcAccessMask;
	VkPipelineStageFlags dstStageFlag;
	VkAccessFlags dstAccessMask;
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities; // for extents
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentMode;
};


// begin and end (with submition) a simple transfer command
VkCommandBuffer beginSingleTimeTransferCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeTransferCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue transferQueue);

// Find memory type index from memory type bit (given by memoryRequirements for example)
uint32_t getMemoryTypeIndexFromMemoryTypeBit(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

// In followings singleCmd[action] means that a command buffer is built, setup and then submitted
// cmd[action] only setup the command buffer (which is requested as parameter)

// Copy buffer to buffer

void cmdCopyBufferToBuffer(VkDevice device, VkCommandBuffer commandBuffer, VkQueue transferQueue
	, Buffer& from, Buffer& to, const BufferCopyInfo* copyInfo, uint32_t copyInfoCount = 1);
void singleCmdCopyBufferToBuffer(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue
	, Buffer& from, Buffer& to, const BufferCopyInfo* copyInfo, uint32_t copyInfoCount = 1);

// Copy buffer to image

void cmdCopyBufferToImage(VkDevice device, VkCommandBuffer commandBuffer, VkQueue transferQueue
	, Buffer& from, Image2D& to);
void singleCmdCopyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue
	, Buffer& from, Image2D& to);

// Image transitions

void cmdTransitionImageLayout(VkDevice device, VkCommandBuffer commandBuffer, VkQueue transferQueue
	, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, const TransitionAccessInfo& accessInfo);
void singleCmdTransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue
	, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, const TransitionAccessInfo& accessInfo);

// Automatically find the access info / stage from image layout
TransitionAccessInfo findTransitionAccessInfo(VkImageLayout oldLayout, VkImageLayout newLayout);

// Use findTransitionAccessInfo to automatically detect the TransitionAccessInfo
void cmdTransitionImageLayout(VkDevice device, VkCommandBuffer commandBuffer, VkQueue transferQueue
	, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
void singleCmdTransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue transferQueue
	, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

// Format selection
bool hasStencilComponent(VkFormat format);
VkFormat findSupportedFormat(VkPhysicalDevice& physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
VkFormat findDepthFormat(VkPhysicalDevice& physicalDevice, size_t* outFormatSize = nullptr, uint16_t* outFormatComponentCount = nullptr);
VkFormat findDepthAndStencilFormat(VkPhysicalDevice& physicalDevice, size_t* outFormatSize = nullptr, uint16_t* outFormatComponentCount = nullptr);

// Device physical selection
bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<const char*>& deviceExtensions);
bool checkSwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
int ratePhysicalDeviceSuitability(VkPhysicalDevice physicalDevice
	, VkSurfaceKHR surface
	, const std::vector<const char*>& requiredDeviceExtensions
	, const VkPhysicalDeviceFeatures& requiredDeviceFeatures
	, bool needPresentSupport
	, VkQueueFlags requestedQueueFlags);

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

// Shader loading
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
std::vector<char> readShaderFile(const std::string& filename);

// Vulkan debug
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData);

VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
bool checkValidationLayerSupport(const std::vector<const char*>& requestedValidationLayers);

// Errors
#define CHECK_VK_THROW_ERROR(item, msg)\
if (item != VK_SUCCESS) {\
	throw std::runtime_error(msg);\
}

#define CHECK_TRUE_THROW_ERROR(item, msg)\
if ((item) != true) {\
	throw std::runtime_error(msg);\
}

// Aligned memory managment

inline uint32_t computeAlignedSize(uint32_t notAlignedSize, uint32_t alignment)
{
	return (notAlignedSize / alignment) * alignment + ((notAlignedSize % alignment) > 0 ? alignment : 0);
}

// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void* alignedAlloc(size_t size, size_t alignment)
{
	void *data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else 
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void alignedFree(void* data)
{
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else 
	free(data);
#endif
}