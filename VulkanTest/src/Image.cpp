#include "Image.h"
#include "VulkanUtils.h"
#include "Buffer.h"


void Image2DCreateInfo::initBase(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
	, uint32_t _width, uint32_t _height, uint16_t _channelCount, size_t _channelsCombinedSize, void* _pixels)
{
	physicalDevice = _physicalDevice;
	device = _device;
	commandPool = _commandPool;
	transferQueue = _transferQueue;

	width = _width;
	height = _height;
	channelCount = _channelCount;
	channelsCombinedSize = _channelsCombinedSize;
	pixels = _pixels;
}

void Image2DCreateInfo::initForColorAttachment(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
	, uint32_t _width, uint32_t _height, uint16_t _channelCount, size_t _channelsCombinedSize, void* _pixels)
{
	initBase(_physicalDevice, _device, _commandPool, _transferQueue
		, _width, _height, _channelCount, _channelsCombinedSize, _pixels);

	usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	tiling = VK_IMAGE_TILING_LINEAR;
	format = VK_FORMAT_R8G8B8A8_UNORM;
	initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
}

void Image2DCreateInfo::initForDepthAttachment(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
	, uint32_t _width, uint32_t _height, bool useStencil, void* _pixels)
{
	size_t formatSize;
	uint16_t formatComponentCount;
	VkFormat depthFormat = findDepthFormat(_physicalDevice, &formatSize, &formatComponentCount);

	initBase(_physicalDevice, _device, _commandPool, _transferQueue
		, _width, _height, formatComponentCount, formatSize, _pixels);

	usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	tiling = VK_IMAGE_TILING_OPTIMAL;
	format = depthFormat;
	initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
}

void Image2DCreateInfo::initForDepthAndStencilAttachment(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
	, uint32_t _width, uint32_t _height, bool useStencil, void* _pixels)
{
	size_t formatSize;
	uint16_t formatComponentCount;
	VkFormat depthFormat = findDepthAndStencilFormat(_physicalDevice, &formatSize, &formatComponentCount);

	initBase(_physicalDevice, _device, _commandPool, _transferQueue
		, _width, _height, formatComponentCount, formatSize, _pixels);

	usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	tiling = VK_IMAGE_TILING_OPTIMAL;
	format = depthFormat;
	initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
}

// Image usaed as sampler and read only from shader with R8G8B8A8_UNORM format
void Image2DCreateInfo::initForTextureSample(VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandPool _commandPool, VkQueue _transferQueue
	, uint32_t _width, uint32_t _height, uint16_t _channelCount, size_t _channelsCombinedSize, void* _pixels)
{
	initBase(_physicalDevice, _device, _commandPool, _transferQueue
		, _width, _height, _channelCount, _channelsCombinedSize, _pixels);

	usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	tiling = VK_IMAGE_TILING_LINEAR;
	format = VK_FORMAT_R8G8B8A8_UNORM;
	initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
}

/////////////////////////////////////////////////////////////////////////////

Image2D::Image2D()
	: width(0)
	, height(0)
	, channelCount(0)
{

}

Image2D::~Image2D()
{
	destroy();
}

void Image2D::create(const Image2DCreateInfo& createInfo)
{
	width = createInfo.width;
	height = createInfo.height;
	channelCount = createInfo.channelCount;
	int pixelsCount = width * height;
	usage = createInfo.usage;
	tiling = createInfo.tiling;
	format = createInfo.format;
	imageLayout = createInfo.imageLayout;

	owningDevice = createInfo.device;

	createImageHandle(createInfo.initialLayout);
	createAndBindMemory(createInfo.physicalDevice);
	if(createInfo.pixels != nullptr)
		transferData(createInfo.physicalDevice, createInfo.commandPool, createInfo.transferQueue, pixelsCount, createInfo.channelsCombinedSize, createInfo.pixels, createInfo.initialLayout);
	
	createView(createInfo.aspectFlags);
}

void Image2D::destroy()
{
	vkDestroyImageView(owningDevice, imageView, nullptr);
	vkDestroyImage(owningDevice, image, nullptr);
	vkFreeMemory(owningDevice, imageMemory, nullptr);
}

uint32_t Image2D::getWidth() const
{
	return width;
}

uint32_t Image2D::getHeight() const
{
	return height;
}

VkImageLayout Image2D::getLayout() const
{
	return imageLayout;
}

VkImage Image2D::getImageHandle() const
{
	return image;
}

VkDeviceMemory Image2D::getImageMemoryHandle() const
{
	return imageMemory;
}

VkImageView Image2D::getImageViewHandle() const
{
	return imageView;
}

VkFormat Image2D::getImageFormat() const
{
	return format;
}

void Image2D::createImageHandle(VkImageLayout initialLayout)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = initialLayout;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(owningDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image !");
	}
}

void Image2D::createAndBindMemory(VkPhysicalDevice physicalDevice)
{
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(owningDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	allocInfo.memoryTypeIndex = getMemoryTypeIndexFromMemoryTypeBit(physicalDevice, memRequirements.memoryTypeBits, memoryFlags);

	if (vkAllocateMemory(owningDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory !");
	}

	vkBindImageMemory(owningDevice, image, imageMemory, 0);
}

void Image2D::transferData(VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue, uint32_t pixelsCount, size_t pixelSize, void* pixels, VkImageLayout preTransferLayout)
{
	Buffer transferBuffer;
	BufferCreateInfo transferBufferCreateInfo;
	transferBufferCreateInfo.itemCount = pixelsCount;
	transferBufferCreateInfo.itemSizeNotAligned = pixelSize;
	transferBufferCreateInfo.owningDevice = owningDevice;
	transferBufferCreateInfo.physicalDevice = physicalDevice;
	transferBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	transferBuffer.create(transferBufferCreateInfo, false);

	BufferCopyInfo transferMapingInfo(pixelSize, pixelsCount, 0, 0);
	transferBuffer.pushDatasToBuffer(physicalDevice, pixels, transferMapingInfo, false);

	{
		VkCommandBuffer commandBuffer = beginSingleTimeTransferCommands(owningDevice, commandPool);
		cmdTransitionImageLayout(owningDevice, commandBuffer, transferQueue, image, preTransferLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		endSingleTimeTransferCommands(owningDevice, commandPool, commandBuffer, transferQueue);
	}
	{
		VkCommandBuffer commandBuffer = beginSingleTimeTransferCommands(owningDevice, commandPool);
		cmdCopyBufferToImage(owningDevice, commandBuffer, transferQueue, transferBuffer, *this);
		endSingleTimeTransferCommands(owningDevice, commandPool, commandBuffer, transferQueue);
	}
	{
		VkCommandBuffer commandBuffer = beginSingleTimeTransferCommands(owningDevice, commandPool);
		cmdTransitionImageLayout(owningDevice, commandBuffer, transferQueue, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout);
		endSingleTimeTransferCommands(owningDevice, commandPool, commandBuffer, transferQueue);
	}
}

void Image2D::createView(VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(owningDevice, &viewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view !");
	}
}
