#include "Pipeline.h"

Pipeline::Pipeline()
	: owningDevice(VK_NULL_HANDLE)
	, pipeline(VK_NULL_HANDLE)
	, pipelineLayout(VK_NULL_HANDLE)
{

}

Pipeline::Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* info)
{
	create(device, info);
}

Pipeline::~Pipeline()
{
	destroy();
}

void Pipeline::create(VkDevice device, const VkGraphicsPipelineCreateInfo* info)
{
	owningDevice = device;

	vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, info, nullptr, &pipeline);
}

void Pipeline::create(VkDevice device, const PipelineInfoRenderableRelated& pipelineInfoRenderableRelated
	, const PipelineInfoMaterialRelated& pipelineInfoMaterialRelated
	, const PipelineInfoSubpassRelated& pipelineInfoSubpassRelated)
{
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

	pipelineInfo.pVertexInputState = &pipelineInfoRenderableRelated.vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &pipelineInfoRenderableRelated.inputAssemblyInfo;

	pipelineInfo.stageCount = pipelineInfoMaterialRelated.stageCount;
	pipelineInfo.pStages = pipelineInfoMaterialRelated.pShaderStages;

	vkCreatePipelineLayout(device, &(pipelineInfoMaterialRelated.pipelineLayoutInfo), nullptr, &pipelineLayout);
	pipelineInfo.layout = pipelineLayout;

	pipelineInfo.pViewportState = &pipelineInfoSubpassRelated.viewportState;
	pipelineInfo.pRasterizationState = &pipelineInfoSubpassRelated.rasterizerInfo;
	pipelineInfo.pMultisampleState = &pipelineInfoSubpassRelated.multisamplingInfo;
	pipelineInfo.pDepthStencilState = &pipelineInfoSubpassRelated.depthStencil;
	pipelineInfo.pColorBlendState = &pipelineInfoSubpassRelated.colorBlendingInfo;
	pipelineInfo.renderPass = pipelineInfoSubpassRelated.renderPass;
	pipelineInfo.subpass = pipelineInfoSubpassRelated.subPass;

	pipelineInfo.pDynamicState = nullptr; //optional

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; //optional
	pipelineInfo.basePipelineIndex = -1; //optional

	create(device, &pipelineInfo);
}

void Pipeline::destroy()
{
	if (owningDevice != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE && pipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(owningDevice, pipelineLayout, nullptr);
		vkDestroyPipeline(owningDevice, pipeline, nullptr);
	}
}

VkPipeline Pipeline::getPipelineHandle()
{
	return pipeline;
}

VkPipelineLayout Pipeline::getPipelineLayout()
{
	return pipelineLayout;
}