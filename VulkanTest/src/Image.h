#pragma once

#include <vulkan/vulkan.hpp>

struct Image2DCreateInfo
{
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkCommandPool commandPool;
	VkQueue transferQueue;

	uint32_t width;
	uint32_t height;
	uint16_t channelCount;
	size_t channelsCombinedSize;

	void* pixels;

	VkImageUsageFlags usage;
	VkImageTiling tiling;
	VkFormat format;
	VkImageLayout initialLayout;
	VkImageLayout imageLayout;
	VkImageAspectFlags aspectFlags;

	Image2DCreateInfo()
		: width(0)
		, height(0)
		, channelCount(0)
		, channelsCombinedSize(0)
		, pixels(nullptr)
		, usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		, tiling(VK_IMAGE_TILING_LINEAR)
		, format(VK_FORMAT_R8G8B8A8_UNORM)
		, initialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
		, imageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		, physicalDevice(VK_NULL_HANDLE)
		, device(VK_NULL_HANDLE)
		, commandPool(VK_NULL_HANDLE)
		, transferQueue(VK_NULL_HANDLE)
		, aspectFlags(0)
	{

	}

	void initBase(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
		, uint32_t _width, uint32_t _height, uint16_t _channelCount, size_t _channelsCombinedSize, void* _pixels);

	void initForColorAttachment(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
		, uint32_t _width, uint32_t _height, uint16_t _channelCount, size_t _channelsCombinedSize, void* _pixels);

	void initForDepthAttachment(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
		, uint32_t _width, uint32_t _height, bool useStencil, void* _pixels);

	void initForDepthAndStencilAttachment(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
		, uint32_t _width, uint32_t _height, bool useStencil, void* _pixels);

	// Image usaed as sampler and read only from shader with R8G8B8A8_UNORM format
	void initForTextureSample(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
		, uint32_t _width, uint32_t _height, uint16_t _channelCount, size_t _channelsCombinedSize, void* _pixels);
};

class Image2D
{
private:

	VkDevice owningDevice;

	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;

	uint32_t width;
	uint32_t height;
	uint16_t channelCount;
	VkImageLayout imageLayout;
	VkImageUsageFlags usage;
	VkImageTiling tiling;
	VkFormat format;

public:
	Image2D();
	~Image2D();
	void create(const Image2DCreateInfo& createInfo);
	void destroy();

	uint32_t getWidth() const;
	uint32_t getHeight() const;
	VkImageLayout getLayout() const;
	VkImage getImageHandle() const;
	VkDeviceMemory getImageMemoryHandle() const;
	VkImageView getImageViewHandle() const;
	VkFormat getImageFormat() const;

private:
	void createImageHandle(VkImageLayout initialLayout);
	void createAndBindMemory(VkPhysicalDevice physicalDevice);
	void transferData(VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, uint32_t itemCount, size_t itemSize, void* pixels, VkImageLayout postTransferLayout);
	void createView(VkImageAspectFlags aspectFlags);
};
