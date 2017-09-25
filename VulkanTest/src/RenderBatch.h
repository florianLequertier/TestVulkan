#pragma once

#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include <map>
#include <set>
#include <vector>

#include "Buffer.h"
#include "Renderable.h"
class Material;
class MaterialInterface;
class Pipeline;
class GraphicsContext;
struct PipelineInfoRenderableRelated;

struct RenderableBufferCreateInfo
{
	RenderableType renderableType;
	uint32_t renderableItemSize;
	uint32_t bufferMaxItemCount;
};

// Renderable buffer will store datas about those inputs
class RenderableBuffer
{
protected:
	Buffer buffer;
	uint32_t size; // in item count
	uint32_t capacity; // in item count

	PipelineInfoRenderableRelated pipelineInfoRenderableRelated;

public:
	// Initialization
	void create(const GraphicsContext& context, size_t itemSizeNotAligned, size_t itemCount, VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	
	// Usage
	bool addDatas(void* alignedDatas, uint32_t itemCount);
	bool addData(void* singleAlignedData);
	void clear();

	// Getters
	const PipelineInfoRenderableRelated& getPipelineInfoRenderableRelated() const;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Generic class handling a secondary level command buffer
class SecondaryGraphicsCommandOwner
{
protected:
	VkDevice owningDevice;
	VkCommandPool commandPool;

	// secondary level command buffer to record batch specific datas
	VkCommandBuffer commandBuffer;

public:
	virtual void create(VkDevice device, VkCommandPool _commandPool)
	{
		owningDevice = device;
		commandPool = _commandPool;

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocateInfo.commandBufferCount = 1;

		vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);
	}

	virtual void destroy()
	{
		vkFreeCommandBuffers(owningDevice, commandPool, 1, &commandBuffer);
	}

	VkCommandBuffer getCommandBuffer() const
	{
		return commandBuffer;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct BatchedRenderableType
{
	RenderableType renderableType;
	std::vector<BatchedMaterial> materialBatch;
	std::unordered_map<Material*, BatchedMaterial*> materialBatchMapping;

	BatchedRenderableType(RenderableType p);
	void addRenderable(Material* material, MaterialInterface* materialInstance, Renderable* renderable);
};

struct BatchedMaterial
{
	Material* material;
	std::vector<BatchedMaterialInterface> materialInterfaceBatch;
	std::unordered_map<MaterialInterface*, BatchedMaterialInterface*> materialInterfaceBatchMapping;

	BatchedMaterial(Material* _material);
	void addRenderable(MaterialInterface* matInterface, Renderable* renderable);
};

struct BatchedMaterialInterface
{
	MaterialInterface* materialInterface;
	std::vector<BatchedRenderable> renderedObjectBatch;
	std::unordered_map<Renderable*, BatchedRenderable*> renderedObjectBatchMapping;

	BatchedMaterialInterface(MaterialInterface* _materialInterface);
	void addRenderable(Renderable* renderable);
};

struct BatchedRenderableInstance
{
	Renderable* renderable;
	std::vector<BatchedRenderableInstance*> renderableInstances;

	BatchedRenderableOwner(Renderable* _renderable);
	void addRenderableInstance(Renderable* renderable);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The batch store each renderable sorting them by renderable type, material and material instance
class RenderBatch : public SecondaryGraphicsCommandOwner
{
private:
	std::vector<RenderableType> allowedRenderables;
	std::vector<BatchedRenderableType> renderableTypeBatch;
	std::unordered_map<RenderableType, RenderableBuffer> renderableBuffers;

public:
	RenderBatch();
	~RenderBatch();
	void create(const GraphicsContext& context, const std::vector<RenderableBufferCreateInfo>& renderableBufferCreateInfos);
	void create(const GraphicsContext& context, const RenderableBufferCreateInfo& renderableBufferCreateInfo);

	// add renderables at each frames based on visibility test
	void addRenderable(Material* mat, MaterialInterface* matInterface, Renderable* renderable);
	// call this function once all renderables have been added to the batch
	void recordRenderCommand(VkRenderPass currentPass, uint32_t currentSubpass);
	// once we have render all renderable for this frame, clear the batch
	void clearBatch();
	void destroy() override;

	// Getters
	bool getPipelineInfoRenderableRelated(RenderableType renderableType, PipelineInfoRenderableRelated& outPipelineInfoRenderableRelated) const;
	void getPipelineInfoRenderableRelated(const std::vector<RenderableType>& renderableTypes, std::vector<PipelineInfoRenderableRelated>& outPipelineInfoRenderableRelated) const;
};
