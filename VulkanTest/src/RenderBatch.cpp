#include "RenderBatch.h"

#include "Renderable.h"
#include "Buffer.h"
#include "Pipeline.h"
#include "GraphicsContext.h"
#include "Material.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// RenderableBuffer 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RenderableBuffer::create(const GraphicsContext& context, size_t itemSizeNotAligned, size_t itemCount, VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
{
	BufferCreateInfo createInfo = BufferCreateInfo::makeAligned(context.getPhysicalDevice()
		, context.getDevice()
		, itemCount
		, itemSizeNotAligned
		, usage);

	buffer.create(createInfo, false);
	capacity = buffer.getItemCount();
}

bool RenderableBuffer::addDatas(void* datas, uint32_t itemCount)
{
	if (size + itemCount >= capacity)
		return false;

	BufferCopyInfo mappingInfo = BufferCopyInfo::makeFromItem(itemCount, 0, size);
	buffer.pushDatasToBuffer(datas, mappingInfo);
	size += itemCount;

	return true;
}

bool RenderableBuffer::addData(void* singleData)
{
	if (size + 1 >= capacity)
		return false;

	BufferCopyInfo mappingInfo = BufferCopyInfo::makeFromItem(1, 0, size);
	buffer.pushDatasToBuffer(singleData, mappingInfo);
	size++;

	return true;
}

void RenderableBuffer::clear()
{
	size = 0;
}

const PipelineInfoRenderableRelated& RenderableBuffer::getPipelineInfoRenderableRelated() const
{
	return pipelineInfoRenderableRelated;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// SecondaryGraphicsCommandOwner 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SecondaryGraphicsCommandOwner::create(VkDevice device, VkCommandPool _commandPool)
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

void SecondaryGraphicsCommandOwner::destroy()
{
	vkFreeCommandBuffers(owningDevice, commandPool, 1, &commandBuffer);
}

VkCommandBuffer SecondaryGraphicsCommandOwner::getCommandBuffer() const
{
	return commandBuffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// Batched types
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BatchedRenderableType::BatchedRenderableType(RenderableType p)
	: renderableType(p)
{

}

void BatchedRenderableType::addRenderable(Material* material, MaterialInterface* materialInstance, Renderable* renderable)
{
	const auto& found = materialBatchMapping.find(material);
	if (found != materialBatchMapping.end())
	{
		found->second->addRenderable(materialInstance, renderable);
	}
	else
	{
		materialBatch.push_back(BatchedMaterial(material));
		materialBatch.back().addRenderable(materialInstance, renderable);
		materialBatchMapping[material] = &materialBatch.back();
	}
}

/////////////////////////////

BatchedMaterial::BatchedMaterial(Material* _material)
	: material(_material)
{}

void BatchedMaterial::addRenderable(MaterialInterface* matInterface, Renderable* renderable)
{
	const auto& found = materialInterfaceBatchMapping.find(matInterface);
	if (found != materialInterfaceBatchMapping.end())
	{
		found->second->addRenderable(renderable);
	}
	else
	{
		materialInterfaceBatch.push_back(BatchedMaterialInterface(matInterface));
		materialInterfaceBatch.back().addRenderable(renderable);
		materialInterfaceBatchMapping[matInterface] = &materialInterfaceBatch.back();
	}
}

/////////////////////////////

BatchedMaterialInterface::BatchedMaterialInterface(MaterialInterface* _materialInterface)
	: materialInterface(_materialInterface)
{}

void BatchedMaterialInterface::addRenderable(Renderable* renderable)
{
	void* renderedObjectPtr = renderable->getRenderedObjectPtr();
	const auto& found = renderedObjectBatchMapping.find(renderedObjectPtr);
	if (found != renderedObjectBatchMapping.end())
	{
		found->second->addRenderable(renderable);
	}
	else
	{
		renderedObjectBatch.push_back(BatchedRenderable(renderable));
		renderedObjectBatch.back().addRenderable(renderable);
		renderedObjectBatchMapping[renderedObjectPtr] = &renderedObjectBatch.back();
	}
}

/////////////////////////////

BatchedRenderable::BatchedRenderable(Renderable* _renderable)
	: renderable(_renderable)
{}

void BatchedRenderable::addRenderable(Renderable* renderable)
{
	renderableInstances.push_back(renderable);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// RenderBatch 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


RenderBatch::RenderBatch()
{}

RenderBatch::~RenderBatch()
{}

void RenderBatch::create(const GraphicsContext& context, const std::vector<RenderableBufferCreateInfo>& renderableBufferCreateInfos)
{
	for (auto& renderableBufferCreateInfo : renderableBufferCreateInfos)
	{
		allowedRenderables.push_back(renderableBufferCreateInfo.renderableType);
		renderableBuffers[renderableBufferCreateInfo.renderableType].create(context, renderableBufferCreateInfo.renderableItemSize, renderableBufferCreateInfo.bufferMaxItemCount);
	}
}

void RenderBatch::create(const GraphicsContext & context, const RenderableBufferCreateInfo & renderableBufferCreateInfo)
{
	allowedRenderables.push_back(renderableBufferCreateInfo.renderableType);
	renderableBuffers[renderableBufferCreateInfo.renderableType].create(context, renderableBufferCreateInfo.renderableItemSize, renderableBufferCreateInfo.bufferMaxItemCount);
}

// add renderables at each frames based on visibility test
void RenderBatch::addRenderable(Material* mat, MaterialInterface* matInterface, Renderable* renderable)
{
	auto& foundRenderableBuffer = renderableBuffers.find(renderable->getRenderableType());
	if (foundRenderableBuffer != renderableBuffers.end())
		foundRenderableBuffer->second.addData(renderable->getMaterialInputDataAligned());
	else
		throw std::runtime_error("invalid renderable type for this batch !");

	////////

	const auto& foundBatch = std::find(renderableTypeBatch.begin(), renderableTypeBatch.end(), renderable->getRenderableType());
	if (foundBatch != renderableTypeBatch.end())
	{
		renderableTypeBatch.push_back(BatchedRenderableType(renderable->getRenderableType()));
		renderableTypeBatch.back().addRenderable(mat, matInterface, renderable);
	}
	else
	{
		foundBatch->addRenderable(mat, matInterface, renderable);
	}
}

// call this function once all renderables have been added to the batch
void RenderBatch::recordRenderCommand(VkRenderPass currentPass, uint32_t currentSubpass)
{
	for (const auto& batch : renderableTypeBatch)
	{
		RenderableType currentRenderableType = batch.renderableType;

		for (const auto& batchedMaterial : batch.materialBatch)
		{
			batchedMaterial.material->cmdBindPipeline(commandBuffer, batch.renderableType, currentPass, currentSubpass);
			batchedMaterial.material->cmdBindGlobalUniforms(commandBuffer);

			for (const auto& batchedMaterialInterface : batchedMaterial.materialInterfaceBatch)
			{
				batchedMaterialInterface.materialInterface->cmdBindLocalUniforms(commandBuffer);

				for (const auto& batchedRenderable : batchedMaterialInterface.renderedObjectBatch)
				{
					batchedRenderable.renderable->cmdbindVBOsAndIBOs(commandBuffer);

					uint32_t renderableIndex = 0;
					for (const auto& renderableInstance : batchedRenderable.renderableInstances)
					{
						uint32_t itemOffset = renderableInstance->getMaterialInputDataAlignedSize() * renderableIndex;
						batchedMaterialInterface.materialInterface->cmdBindRenderableUniforms(commandBuffer, currentRenderableType, itemOffset);
						//renderable->cmdbindRenderableUniforms(commandBuffer);
						renderable->cmdDraw(commandBuffer);
					}
				}
			}
		}
	}
}

// once we have render all renderable for this frame, clear the batch
void RenderBatch::clearBatch()
{
	renderableTypeBatch.clear();

	for (auto& buffer : renderableBuffers)
	{
		buffer.second.clear();
	}
}

void RenderBatch::destroy()
{
	SecondaryGraphicsCommandOwner::destroy();
	clearBatch();
}

bool RenderBatch::getPipelineInfoRenderableRelated(RenderableType renderableType, PipelineInfoRenderableRelated& outPipelineInfoRenderableRelated) const
{
	auto found = renderableBuffers.find(renderableType);
	if (found != renderableBuffers.end())
	{
		outPipelineInfoRenderableRelated = found->second.getPipelineInfoRenderableRelated();
		return true;
	}
	else
		return false;
}

void RenderBatch::getPipelineInfoRenderableRelated(const std::vector<RenderableType>& renderableTypes, std::vector<PipelineInfoRenderableRelated>& outPipelineInfoRenderableRelated) const
{
	outPipelineInfoRenderableRelated.clear();

	for (auto& renderableType : renderableTypes)
	{
		auto found = renderableBuffers.find(renderableType);
		if (found != renderableBuffers.end())
			outPipelineInfoRenderableRelated.push_back(found->second.getPipelineInfoRenderableRelated());
	}
}
