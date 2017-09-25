#include "Renderer.h"
#include "Mesh.h"
#include "Material.h"
#include "RenderBatch.h"

RenderPassData createDeferredRenderPass()
{
	// TODO
}

RenderPassData createHorizontalBloomRenderPass()
{

}

RenderPassData createVerticalBloomRenderPass()
{

}

class LightedGeometryRenderNode final : public RenderNode
{
protected:
	void createRenderPasses() override
	{
		const size_t passIndex = renderPasses.size();
		renderPasses.push_back(createDeferredRenderPass());

	}
};

class BloomRenderNode final : public RenderNode
{
protected:
	void createRenderPasses() override
	{
		const size_t horizontalBloomPassIndex = renderPasses.size();
		renderPasses.push_back(createHorizontalBloomRenderPass());

		const size_t verticalBloomPassIndex = renderPasses.size();
		renderPasses.push_back(createVerticalBloomRenderPass());
		makeNodeWaitOtherPasses(verticalBloomPassIndex, { horizontalBloomPassIndex });
	}
};

void testRenderer()
{
	StaticMesh mesh;

	// deferred materials
	Material gPassMat;
	Material lightingMat;

	// post process materials
	Material bloomMat;

	Renderer renderer;
	renderer.create();

	/////////////////////////////////////////////////

	std::unique_ptr<LightedGeometryRenderNode> lightedGeometryRenderNode;
	lightedGeometryRenderNode->create(renderer.getGraphicsContext().getDevice(), renderer.getGraphicsContext().getCommandPool());

	std::vector<RenderableType> lightedGeometryRenderableTypes = { RenderableType::PIPELINE_TYPE_BILLBOARD
		, RenderableType::PIPELINE_TYPE_SKELETAL_MESH
		, RenderableType::PIPELINE_TYPE_INSTANCED_STATIC_MESH
		, RenderableType::PIPELINE_TYPE_STATIC_MESH
	};
	std::vector<RenderableBufferCreateInfo> lightedGeometryRenderableBufferCreateInfo = {
		{ RenderableType::PIPELINE_TYPE_BILLBOARD, sizeof(BillboardMaterialInputDatas), 100 }
		,{ RenderableType::PIPELINE_TYPE_SKELETAL_MESH, sizeof(SkeletonMeshMaterialInputDatas), 10 }
		,{ RenderableType::PIPELINE_TYPE_INSTANCED_STATIC_MESH, sizeof(InstancedStaticMeshMaterialInputDatas), 1000 }
		,{ RenderableType::PIPELINE_TYPE_STATIC_MESH, sizeof(StaticMeshMaterialInputDatas), 100 }
	};

	std::shared_ptr<RenderBatch> geometryBatch;
	geometryBatch->create(renderer.getGraphicsContext(), lightedGeometryRenderableBufferCreateInfo);
	lightedGeometryRenderNode->setBatchForSubPass(0, 0, geometryBatch);

	std::shared_ptr<RenderBatch> lightBatch;
	lightBatch->create(renderer.getGraphicsContext(), { RenderableType::PIPELINE_TYPE_BLIT_QUAD, 0, 1 });
	lightedGeometryRenderNode->setBatchForSubPass(0, 0, lightBatch);

	std::vector<PipelineInfoRenderableRelated> pipelineInfosRenderableRelated;
	geometryBatch->getPipelineInfoRenderableRelated(lightedGeometryRenderableTypes, pipelineInfosRenderableRelated);

	// make gPassMat valid for the gPass subPass
	gPassMat.setMaterialValidFor(pipelineInfosRenderableRelated, lightedGeometryRenderNode->getPipelineInfoSubPassRelated(0, 0));

	// make lightingMat valid for the lighting subPass
	gPassMat.setMaterialValidFor(pipelineInfosRenderableRelated, lightedGeometryRenderNode->getPipelineInfoSubPassRelated(0, 1));

	/////////////////////////////////////////////////

	// make a post process node
	std::unique_ptr<BloomRenderNode> bloomRenderNode;
	bloomRenderNode->create(renderer.getGraphicsContext().getDevice(), renderer.getGraphicsContext().getCommandPool());

	std::shared_ptr<RenderBatch> postProcessBatch;
	postProcessBatch->create(renderer.getGraphicsContext(), { RenderableType::PIPELINE_TYPE_BLIT_QUAD, 0, 1 });
	lightedGeometryRenderNode->setBatchForAllSubPasses(postProcessBatch);

	postProcessBatch->getPipelineInfoRenderableRelated(RenderableType::PIPELINE_TYPE_BLIT_QUAD, pipelineInfosRenderableRelated);

	// make sur the bloom material is valid for all sub passes and the PIPELINE_TYPE_BLIT_QUAD renderable type
	for (int i = 0; i < bloomRenderNode->getRenderPassCount(); i++)
	{
		for (int j = 0; j < bloomRenderNode->getSubPassCount(); j++)
		{
			bloomMat->setMaterialValidFor(pipelineInfosRenderableRelated, bloomRenderNode->getPipelineInfoSubPassRelated(i, j));
		}
	}

	/////////////////////////////////////////////////

	// combine nodes inside a render process
	std::unique_ptr<RenderProcess> sceneRenderProcess;
	sceneRenderProcess->addRenderNode(lightedGeometryRenderNode);
	sceneRenderProcess->addRenderNode(bloomRenderNode); // must wait the node 0

	renderer.addRenderProcess(sceneRenderProcess);

	// once all render processes have been added
	renderer.setupProcesses();
	renderer.recordCommands();

	/////////////////////////////////////////////////



	// Game loop : 

	// Game update -> update positions for example

	// We need to update items inside the batch, then record the batch command again
	sceneBatch.clear();
	sceneBatch.addRenderable(mat, mesh);
	...
		sceneBatch.recordRenderCommand();

	renderer.submitProcesses();

}