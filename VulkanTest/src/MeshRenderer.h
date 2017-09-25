#pragma once

#include "Renderable.h"
#include "glm/glm.hpp"

class StaticMesh;
class MaterialInterface;
class GraphicsContext;

class MeshRenderer : public IRenderableInstance
{
private:

	StaticMeshMaterialInputDatas* inputData;
	uint32_t dataAlignedSize;

	StaticMesh* mesh;
	MaterialInterface* material;

public:

	MeshRenderer(const GraphicsContext& context);
	~MeshRenderer();

// IRenderableInstance implementation

	void updateModelMatrix(const glm::mat4& newTransform);
	void cmdbindVBOsAndIBOs(VkCommandBuffer commandBuffer) override;
	void cmdDraw(VkCommandBuffer commandBuffer) override;
	void* getMaterialInputDataAligned() override;
	uint32_t getMaterialInputDataAlignedSize() override;
	void* getRenderablePtr() override;
	RenderableType getRenderableType() const override;
	RenderableTypeFlag getRenderableTypeFlag() const override;
};
