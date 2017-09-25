#pragma once

#include <vulkan/vulkan.hpp>

struct BufferCreateInfo
{
	VkBufferUsageFlags usage;
	VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkDevice owningDevice;
	VkPhysicalDevice physicalDevice;
	uint32_t itemCount;
	uint32_t itemSizeNotAligned;
	bool useAlignment;

	static BufferCreateInfo makeAligned(VkPhysicalDevice physicalDevice
		, VkDevice device
		, uint32_t itemCount
		, uint32_t itemNotAlignedSize
		, VkBufferUsageFlags bufferUsage
	)
	{
		BufferCreateInfo createInfo = {};
		createInfo.usage = bufferUsage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.owningDevice = device;
		createInfo.physicalDevice = physicalDevice;
		createInfo.itemCount = itemCount;
		createInfo.itemSizeNotAligned = 0;
		createInfo.itemSizeNotAligned = itemNotAlignedSize;
		createInfo.useAlignment = true;

		return createInfo;
	}

	static BufferCreateInfo makeNotAligned(VkPhysicalDevice physicalDevice
		, VkDevice device
		, uint32_t itemCount
		, uint32_t itemNotAlignedSize
		, VkBufferUsageFlags bufferUsage
	)
	{
		BufferCreateInfo createInfo = {};
		createInfo.usage = bufferUsage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.owningDevice = device;
		createInfo.physicalDevice = physicalDevice;
		createInfo.itemCount = itemCount;
		createInfo.itemSizeNotAligned = itemNotAlignedSize;
		createInfo.itemSizeAligned = 0;
		createInfo.useAlignment = false;

		return createInfo;
	}
};

struct BufferCopyInfo
{
	VkDeviceSize itemCount;
	VkDeviceSize srcItemCountOffset = 0;
	VkDeviceSize dstItemCountOffset = 0;

	//BufferCopyInfo(uint32_t _itemCount, uint32_t _srcItemCountOffset = 0, uint32_t _dstItemCountOffset = 0)
	//	: itemCount(_itemCount)
	//	, srcItemCountOffset(_srcItemCountOffset)
	//	, dstItemCountOffset(_dstItemCountOffset)
	//{
	//	if (itemCount == 0) {
	//		throw std::invalid_argument("invalid size or typeSizeNotAligned argument in BufferCopyInfo()");
	//	}
	//}

	static BufferCopyInfo makeFromItem(uint32_t _itemCount, uint32_t _srcItemCountOffset = 0, uint32_t _dstItemCountOffset = 0)
	{
		BufferCopyInfo copyInfo = {};
		copyInfo.itemCount = _itemCount;
		copyInfo.srcItemCountOffset = _srcItemCountOffset;
		copyInfo.dstItemCountOffset = _dstItemCountOffset;

		return copyInfo;
	}
};

class Buffer
{
private:
	VkDevice owningDevice;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBufferUsageFlags usage;
	VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	bool useAlignment = false;

	uint32_t itemCount = 0;
	uint32_t size = 0;
	size_t itemSizeNotAligned = 0;

	uint32_t sizeAligned = 0;
	size_t itemSizeAligned = 0;

public:

	Buffer();
	~Buffer();
	void create(const BufferCreateInfo& createInfo, bool useStaging = false);

	void pushDatasToBuffer(const void* datas, const BufferCopyInfo& mappingInfo
		, bool useSharing = false, VkPhysicalDevice physicalDevice = VK_NULL_HANDLE, VkCommandPool commandPool = VK_NULL_HANDLE, VkQueue transferQueue = VK_NULL_HANDLE);

	void destroy();
	const VkBuffer* getBufferHandle() const;
	const VkDeviceMemory* getMemoryHandle() const;
	uint32_t getItemCount() const;
	uint32_t getSize() const;
	size_t getItemSizeNotAligned() const;
	size_t getItemSizeAligned() const;

private:

	void createBufferHandle();
	void createAndBindMemory(VkPhysicalDevice physicalDevice, bool useStaging = false);
	void mapDatas(const void* datas, const BufferCopyInfo& mappingInfo);

};
