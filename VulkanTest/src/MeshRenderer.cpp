#include "MeshRenderer.h"

#include "Mesh.h"
#include "Material.h"
#include "GraphicsContext.h"

MeshRenderer::MeshRenderer(const GraphicsContext& context)
{
	uint32_t alignment = context.getUBOAlignement();
	dataAlignedSize = computeAlignedSize(sizeof(StaticMeshMaterialInputDatas), alignment);
	inputData = (StaticMeshMaterialInputDatas*)alignedAlloc(dataAlignedSize, alignment);
}

MeshRenderer::~MeshRenderer()
{
	alignedFree(inputData);
}

void MeshRenderer::updateModelMatrix(const glm::mat4& newTransform)
{
	inputData->MVP = newTransform;
}

void MeshRenderer::cmdbindVBOsAndIBOs(VkCommandBuffer commandBuffer)
{
	mesh->cmdBindVBOsAndIBOs(commandBuffer);
}

void MeshRenderer::cmdDraw(VkCommandBuffer commandBuffer)
{
	mesh->cmdDraw();
}

void* MeshRenderer::getMaterialInputDataAligned()
{
	return inputData;
}

uint32_t MeshRenderer::getMaterialInputDataAlignedSize()
{
	return dataAlignedSize;
}

void* MeshRenderer::getRenderablePtr()
{
	return mesh;
}

RenderableType MeshRenderer::getRenderableType() const
{
	return mesh->getRenderableType();
}

RenderableTypeFlag MeshRenderer::getRenderableTypeFlag() const
{
	return mesh->getRenderableTypeFlag();
}