#pragma once

#include <vulkan/vulkan.hpp>

#include "Renderable.h"

struct PipelineInfoSubpassRelated
{
	VkRenderPass renderPass;
	uint32_t subPass;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizerInfo;
	VkPipelineMultisampleStateCreateInfo multisamplingInfo;
	VkPipelineColorBlendStateCreateInfo colorBlendingInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
};

struct PipelineInfoMaterialRelated
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo;
	uint32_t stageCount;
	VkPipelineShaderStageCreateInfo* pShaderStages;
};

struct PipelineInfoRenderableRelated
{
	RenderableType renderableType;
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
};

class Pipeline
{
private:
	VkDevice owningDevice;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

public:
	Pipeline();
	Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* info);
	~Pipeline();

	void create(VkDevice device, const VkGraphicsPipelineCreateInfo* info);
	void create(VkDevice device, const PipelineInfoRenderableRelated& pipelineInfoRenderableRelated
								, const PipelineInfoMaterialRelated& pipelineInfoMaterialRelated
								, const PipelineInfoSubpassRelated& pipelineInfoSubpassRelated);
	void destroy();

	VkPipeline getPipelineHandle();
	VkPipelineLayout getPipelineLayout();

};