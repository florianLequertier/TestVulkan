#include "Buffer.h"
#include "VulkanUtils.h"

#include "vulkan/vulkan.hpp"

Buffer::Buffer()
	: itemCount(0)
	, size(0)
	, sharingMode(VK_SHARING_MODE_EXCLUSIVE)
{}

Buffer::~Buffer()
{
	destroy();
}

void Buffer::create(const BufferCreateInfo& createInfo, bool useStaging)
{
	if (itemCount != 0)
		destroy();

	// get alignment
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(createInfo.physicalDevice, &physicalDeviceProperties);
	size_t alignment = physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

	usage = useStaging ? (createInfo.usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT) : createInfo.usage;
	sharingMode = createInfo.sharingMode;
	itemCount = createInfo.itemCount;
	itemSizeNotAligned = createInfo.itemSizeNotAligned;
	itemSizeAligned = computeAlignedSize(createInfo.itemSizeNotAligned, alignment);
	size = itemSizeNotAligned * createInfo.itemCount;
	sizeAligned = itemSizeAligned * createInfo.itemCount;
	owningDevice = createInfo.owningDevice;
	useAlignment = createInfo.useAlignment;

	createBufferHandle();
	createAndBindMemory(createInfo.physicalDevice, useStaging);
}

void Buffer::pushDatasToBuffer(const void* datas, const BufferCopyInfo& mappingInfo
	, bool useSharing, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue transferQueue)
{
	if (useSharing)
	{
		Buffer stagingBuffer;
		BufferCreateInfo stagingCreateInfo;
		stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		stagingCreateInfo.owningDevice = owningDevice;
		stagingCreateInfo.physicalDevice = physicalDevice;
		stagingCreateInfo.itemCount = mappingInfo.itemCount;
		stagingCreateInfo.itemSizeNotAligned = itemSizeNotAligned;
		stagingCreateInfo.useAlignment = useAlignment;
		stagingBuffer.create(stagingCreateInfo, false);

		BufferCopyInfo stagingMapingInfo = mappingInfo;
		stagingMapingInfo.srcItemCountOffset = 0;
		stagingBuffer.pushDatasToBuffer(datas, stagingMapingInfo, false);

		singleCmdCopyBufferToBuffer(owningDevice, commandPool, transferQueue, stagingBuffer, *this, &mappingInfo);
	}
	else
	{
		mapDatas(datas, mappingInfo);
	}
}

void Buffer::destroy()
{
	if (itemCount == 0)
		return;

	vkDestroyBuffer(owningDevice, vertexBuffer, nullptr);
	vkFreeMemory(owningDevice, vertexBufferMemory, nullptr);
	itemCount = 0;
	size = 0;
}

const VkBuffer* Buffer::getBufferHandle() const
{
	return &vertexBuffer;
}

const VkDeviceMemory* Buffer::getMemoryHandle() const
{
	return &vertexBufferMemory;
}

uint32_t Buffer::getItemCount() const
{
	return itemCount;
}

uint32_t Buffer::getSize() const
{
	return size;
}

size_t Buffer::getItemSizeNotAligned() const
{
	return itemSizeNotAligned;
}

void Buffer::createBufferHandle()
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = sharingMode;

	if (vkCreateBuffer(owningDevice, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer !");
	}
}

void Buffer::createAndBindMemory(VkPhysicalDevice physicalDevice, bool useStaging)
{
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(owningDevice, vertexBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	VkMemoryPropertyFlags memoryFlags = useStaging ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	allocInfo.memoryTypeIndex = getMemoryTypeIndexFromMemoryTypeBit(physicalDevice, memRequirements.memoryTypeBits, memoryFlags);

	if (vkAllocateMemory(owningDevice, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory !");
	}

	vkBindBufferMemory(owningDevice, vertexBuffer, vertexBufferMemory, 0);
}

void Buffer::mapDatas(const void* datas, const BufferCopyInfo& mappingInfo)
{
	//assert(mappingInfo.dstOffset + mappingInfo.size <= size);
	//assert(mappingInfo.srcOffset + mappingInfo.size <= datas.size())// *sizeof(datas[0]));

	const uint32_t usedItemSize = useAlignment ? itemSizeAligned : itemSizeNotAligned;
	const char* fromPtr = reinterpret_cast<const char*>(datas) + (itemSizeAligned * mappingInfo.srcItemCountOffset);
	const size_t size = mappingInfo.itemCount * itemSizeAligned;
	const VkDeviceSize dstOffset = mappingInfo.dstItemCountOffset * itemSizeAligned;

	void* data;
	vkMapMemory(owningDevice, vertexBufferMemory, dstOffset, size, 0, &data);
	memcpy(data, (void*)fromPtr, (size_t)size);
	vkUnmapMemory(owningDevice, vertexBufferMemory);
}
