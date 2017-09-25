#pragma once

#include <vulkan/vulkan.hpp>

#include <stdlib.h>

// Each renderable type correspond to a certain input layout inside vertex shader

enum RenderableType
{
	PIPELINE_TYPE_STATIC_MESH,
	PIPELINE_TYPE_SKELETAL_MESH,
	PIPELINE_TYPE_BILLBOARD,
	PIPELINE_TYPE_BLIT_QUAD,
	PIPELINE_TYPE_INSTANCED_STATIC_MESH
};

enum RenderableTypeFlag
{
	PIPELINE_TYPE_STATIC_MESH = 0,
	PIPELINE_TYPE_SKELETAL_MESH = 1 << 0,
	PIPELINE_TYPE_BILLBOARD = 1 << 1,
	PIPELINE_TYPE_BLIT_QUAD = 1 << 2,
	PIPELINE_TYPE_INSTANCED_STATIC_MESH = 1 << 3
};

struct StaticMeshMaterialInputDatas
{
	glm::mat4 MVP;
};

struct InstancedStaticMeshMaterialInputDatas
{
	//TODO
};

struct SkeletonMeshMaterialInputDatas
{
	glm::mat4 MVP;
	glm::mat4 bones[100];
};

struct BillboardMaterialInputDatas
{
	glm::mat4 position;
};

// A renderable is responsible for binding the VBOs and IBOs of the rendered object
// and call the draw command
// It can give you the data corresponding to renderable material input 
// (i.e : uniforms for each instance of renderable and passed to vertex shader (like the MVP matrix, the bones transforms, ...) )
class Renderable
{
protected:
	RenderableType renderableType;
	RenderableTypeFlag renderableTypeFlag;

public:
	Renderable(RenderableType _renderableType, RenderableTypeFlag _renderableTypeFlag)
		: renderableType(_renderableType)
		, renderableTypeFlag(_renderableTypeFlag)
	{}

	inline RenderableType getRenderableType() const
	{
		return renderableType;
	}
	inline RenderableTypeFlag getRenderableTypeFlag() const
	{
		return renderableTypeFlag;
	}

	virtual void cmdbindVBOsAndIBOs(VkCommandBuffer commandBuffer) = 0;
	virtual void cmdDraw(VkCommandBuffer commandBuffer) = 0;
};

class IRenderableInstance
{
public:
	virtual void cmdbindVBOsAndIBOs(VkCommandBuffer commandBuffer) = 0;
	virtual void cmdDraw(VkCommandBuffer commandBuffer) = 0;

	virtual void* getRenderablePtr() = 0;

	virtual RenderableType getRenderableType() const = 0;
	virtual RenderableTypeFlag getRenderableTypeFlag() const = 0;

	// Depending on RenderableType, void* can represent 
	// StaticMeshMaterialInputDatas, SkeletonMeshMaterialInputDatas, BillboardMaterialInputDatas
	virtual void* getMaterialInputDataAligned() = 0;
	// The data size represent the size of the previous data, taking into account the alignment
	virtual uint32_t getMaterialInputDataAlignedSize() = 0;
};

// You can use it with a simple blit shader coverring all screen
class BlitQuad final : public Renderable
{
public:

	void cmdbindVBOsAndIBOs(VkCommandBuffer commandBuffer) override
	{
		// no ibo nor vbo for the blit quad
	}

	void cmdDraw(VkCommandBuffer commandBuffer) override
	{
		// Simply draw 4 vertices, the vertex shader will automatically place them
		vkCmdDraw(commandBuffer, 4, 1, 0, 0);
	}

	void* getRenderedObjectPtr() override
	{
		return this;
	}

	void* getData()
	{
		// no data to send to buffer because we need no uniform
		return nullptr;
	}
};
