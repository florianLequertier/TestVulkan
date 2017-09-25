#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <unordered_map>

#include "VulkanUtils.h"
#include "MaterialInputs.h"
#include "Pipeline.h"

class GraphicsContext;

class MaterialInterface
{
public:
	virtual void createGPUSide(const GraphicsContext& context) = 0;
	virtual void destroyGPUSide() = 0;

	virtual void createDescriptorPool(const GraphicsContext& context) = 0;

	virtual void cmdBindPipeline(VkCommandBuffer commandBuffer, RenderableType renderableType, VkRenderPass currentPass, uint32_t currentSubpass) = 0;
	virtual void cmdBindGlobalUniforms(VkCommandBuffer commandBuffer) = 0;
	virtual void cmdBindLocalUniforms(VkCommandBuffer commandBuffer) = 0;
	virtual void cmdBindRenderableUniforms(VkCommandBuffer commandBuffer, size_t itemOffset) = 0;
};

struct MaterialPipelineKey
{
	RenderableType renderableType;
	VkRenderPass renderPass;
	uint32_t subPass;
};

class Material final : public MaterialInterface
{
private:
	std::string vertexShaderPath;
	std::string fragmentShaderPath;

	VkDevice owningDevice;

	MaterialInputSet materialGlobalInputs;
	MaterialInputSet materialLocalInputs;
	std::unordered_map<RenderableType, MaterialInputSet> materialRenderableInputs;
	//MaterialDescriptors<3> materialData;
	VkDescriptorPool descriptorPoolGlobalInputs;
	VkDescriptorPool descriptorPoolLocalInputs;
	std::unordered_map<RenderableType, VkDescriptorPool> descriptorPoolRenderableInputs;

	std::unordered_map<materialPipelineKey, std::unique_ptr<Pipeline>> pipelines;

	std::vector<MaterialInstance*> instances;

public:
	// Il reste a creer le pipeline et l'ajouter par ref au renderer

	Material()
	{

	}

	~Material()
	{
		if (owningDevice != VK_NULL_HANDLE)
		{
			destroyGPUSide();
		}
	}

	void createGPUSide(const GraphicsContext& context) override
	{
		owningDevice = context.getDevice();

		createDescriptorPool(context);

		materialGlobalInputs.createGPUSide(context, descriptorPoolGlobalInputs);
		materialLocalInputs.createGPUSide(context, descriptorPoolLocalInputs);
		for (auto& pair_type_input : materialRenderableInputs)
		{
			auto& found = descriptorPoolRenderableInputs.find(pair_type_input.first);
			assert(found != descriptorPoolRenderableInputs.end());
			pair_type_input.second.createGPUSide(context, found->second);
		}
	}

	void setMaterialValidFor(const PipelineInfoRenderableRelated& pipelineInfoRenderableRelated, const PipelineInfoSubpassRelated& pipelineInfoSubpassRelated)
	{
		MaterialPipelineKey key = { pipelineInfoRenderableRelated.renderableType, pipelineInfoSubpassRelated.renderPass, pipelineInfoSubpassRelated.subPass };
		createPipeline(key, pipelineInfoRenderableRelated, pipelineInfoSubpassRelated);
	}

	void createPipeline(const MaterialPipelineKey& key, const PipelineInfoRenderableRelated& pipelineInfoRenderableRelated, const PipelineInfoSubpassRelated& pipelineInfoSubpassRelated)
	{
		PipelineInfoMaterialRelated pipelineInfoMaterialRelated = {};

		// Create the PipelineInfoMaterialRelated 
		VkDescriptorSetLayout setLayouts[3] = { materialGlobalInputs.getDescriptorSetLayout(), materialLocalInputs.getDescriptorSetLayout(), pair_type_input.second.getDescriptorSetLayout() };
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;
		pipelineLayoutInfo.setLayoutCount = 3;
		pipelineLayoutInfo.pSetLayouts = setLayouts;

		auto vertShaderCode = readShaderFile(vertexShaderPath);
		auto fragShaderCode = readShaderFile(fragmentShaderPath);

		VkShaderModule vertShaderModule = createShaderModule(owningDevice, vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(owningDevice, fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		pipelineInfoMaterialRelated.pipelineLayoutInfo = pipelineLayoutInfo;
		pipelineInfoMaterialRelated.stageCount = 2;
		pipelineInfoMaterialRelated.pShaderStages = shaderStages;

		// create the pipeline combining all infos
		auto pipelineRef = std::make_unique<Pipeline>();
		pipelineRef->create(owningDevice, pipelineInfoRenderableRelated, pipelineInfoMaterialRelated, pipelineInfoSubpassRelated);
		pipelines[key] = std::move(pipelineRef);

		// destroy shader modules
		vkDestroyShaderModule(owningDevice, vertShaderModule, nullptr);
		vkDestroyShaderModule(owningDevice, fragShaderModule, nullptr);
	}

	void destroyGPUSide() override
	{
		materialGlobalInputs.destroyGPUSide(owningDevice, descriptorPoolGlobalInputs);
		materialLocalInputs.destroyGPUSide(owningDevice, descriptorPoolLocalInputs);
		for (auto& pair_type_input : materialRenderableInputs)
		{
			auto& found = descriptorPoolRenderableInputs.find(pair_type_input.first);
			pair_type_input.second.destroyGPUSide(owningDevice, found->second);
		}

		vkDestroyDescriptorPool(owningDevice, descriptorPoolGlobalInputs, nullptr);
		vkDestroyDescriptorPool(owningDevice, descriptorPoolLocalInputs, nullptr);
		for(auto& pair_type_pool : descriptorPoolRenderableInputs)
			vkDestroyDescriptorPool(owningDevice, pair_type_pool.second, nullptr);
	}

	void cmdBindPipeline(VkCommandBuffer commandBuffer, RenderableType renderableType, VkRenderPass currentPass, uint32_t currentSubpass) override
	{
		auto& pipeline = pipelines[MaterialPipelineKey{ renderableType, currentPass, currentSubpass }];
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineHandle());
	}

	void cmdBindGlobalUniforms(VkCommandBuffer commandBuffer) override
	{
		VkDescriptorSet set = materialGlobalInputs.getDescriptorSet();
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineRef->getPipelineLayout(), 0, 1, &set, 0, 0);
	}

	void cmdBindLocalUniforms(VkCommandBuffer commandBuffer) override
	{
		VkDescriptorSet set = materialLocalInputs.getDescriptorSet();
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineRef->getPipelineLayout(), 0, 1, &set, 0, 0);
	}

	void cmdBindRenderableUniforms(VkCommandBuffer commandBuffer, RenderableType renderableType, uint32_t itemOffset) override
	{
		
		auto& foundInput = materialRenderableInputs.find(renderableType);
		auto& foundPipeline = pipelines.find(renderableType);

		if (foundInput != materialRenderableInputs.end() && foundPipeline != pipelines.end())
		{
			VkDescriptorSet set = foundInput->second.getDescriptorSet();
			VkPipelineLayout pipelineLayout = foundPipeline->second->getPipelineLayout();
			uint32_t offsets[] = { itemOffset };
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &set, 1, &offsets);
		}
	}

	void createDescriptorPool(const GraphicsContext& context) override
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		///////////////////////////////
		materialGlobalInputs.getPoolSizes(poolSizes);

		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1;

		if (vkCreateDescriptorPool(context.getDevice(), &poolInfo, nullptr, &descriptorPoolGlobalInputs) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool !");
		}

		///////////////////////////////
		materialLocalInputs.getPoolSizes(poolSizes);

		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1;

		if (vkCreateDescriptorPool(context.getDevice(), &poolInfo, nullptr, &descriptorPoolLocalInputs) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool !");
		}

		///////////////////////////////
		for (auto& pair_type_input : materialRenderableInputs)
		{
			pair_type_input.second.getPoolSizes(poolSizes);

			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = 1;

			descriptorPoolRenderableInputs[pair_type_input.first] = VK_NULL_HANDLE;
			if (vkCreateDescriptorPool(context.getDevice(), &poolInfo, nullptr, &descriptorPoolRenderableInputs[pair_type_input.first]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor pool !");
			}
		}

	}
};

class MaterialInstance final : public MaterialInterface
{
private:
	VkDevice owningDevice;

	Material* parentMaterial;

	MaterialInputSet materialLocalInputs;
	VkDescriptorPool descriptorPoolLocalInputs;

public:
	MaterialInstance()
		: owningDevice(VK_NULL_HANDLE)
	{}

	~MaterialInstance()
	{
		if (owningDevice != VK_NULL_HANDLE)
		{
			destroyGPUSide();
		}
	}

	void createGPUSide(const GraphicsContext& context) override
	{
		owningDevice = context.getDevice();

		materialLocalInputs.createGPUSide(context);

		std::array<MaterialInputSet*, 3> inputSets = { &materialGlobalInputs , &materialLocalInputs, &materialRenderableInputs };
		materialData.createGPUSide(context, inputSets);
	}

	void destroyGPUSide() override
	{
		materialGlobalInputs.destroyGPUSide(owningDevice);
		materialLocalInputs.destroyGPUSide(owningDevice);
		materialRenderableInputs.destroyGPUSide(owningDevice);

		vkDestroyDescriptorPool(owningDevice, descriptorPool, nullptr);
	}

	void cmdBindPipeline(VkCommandBuffer commandBuffer) override
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineRef->getPipelineHandle());
	}

	void cmdBindGlobalUniforms(VkCommandBuffer commandBuffer) override
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineRef->getPipelineLayout(), 0, 1, &materialData.descriptorSets[0], 0, 0);
	}

	void cmdBindLocalUniforms(VkCommandBuffer commandBuffer) override
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineRef->getPipelineLayout(), 0, 1, &materialData.descriptorSets[1], 0, 0);
	}

	void cmdBindRenderableUniforms(VkCommandBuffer commandBuffer, size_t itemOffset) override
	{
		uint32_t memoryOffset = itemOffsetToMemoryOffset(itemOffset); //TODO
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineRef->getPipelineLayout(), 0, 1, &materialData.descriptorSets[2], 1, &memoryOffset);
	}

	void createDescriptorPool(const GraphicsContext& context) override
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		///////////////////////////////
		materialLocalInputs.getPoolSizes(poolSizes);

		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 1;

		if (vkCreateDescriptorPool(context.getDevice(), &poolInfo, nullptr, &descriptorPoolLocalInputs) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool !");
		}
	}
};

//
//// We create 3 pools to allocate 3 descriptor sets.
//// The global one handle global parameter of material (time, ...)
//// The local one handle parameter local to material instance ( most of textures, floats, ...)
//// The renderable one handle parameter describing renderable instance (transformations, ...)
//template<int inputSetCount = 3>
//struct MaterialDescriptors
//{
//	std::array<VkDescriptorSetLayout, inputSetCount> descriptorSetLayouts;
//	std::array<VkDescriptorPool, inputSetCount> descriptorPools;
//	std::array<VkDescriptorSet, inputSetCount> descriptorSets;
//
//	void createGPUSide(const GraphicsContext& context, const std::array<MaterialInputSet*, inputSetCount>& inputSets)
//	{
//		int index = 0;
//		for (auto inputSet : inputSets)
//		{
//			createDescriptorPool(context, index, *inputSet, inputSetCount);
//			index++;
//		}
//
//		index = 0;
//		for (auto inputSet : inputSets)
//		{
//			createDescriptorSetLayout(context, index, *inputSet);
//			index++;
//		}
//
//		updateAllDescriptorSetsWrite(context);
//
//		// allocate the set at index i using the descriptorSetLayout[i] and the descriptorSetPool[i]
//		for (int i = 0; i < inputSetCount; i++)
//		{
//			allocateDescriptorSet(context, i);
//		}
//	}
//
//	void destroyGPUSide(const GraphicsContext& context)
//	{
//		for (int i = 0; i < inputSetCount; i++)
//		{
//			vkFreeDescriptorSets(context.getDevice(), descriptorPools[i], descriptorSets[i], nullptr);
//		}
//
//		for (int i = 0; i < inputSetCount; i++)
//		{
//			vkDestroyDescriptorPool(context.getDevice(), descriptorPools[i], nullptr);
//		}
//
//		for (int i = 0; i < inputSetCount; i++)
//		{
//			vkDestroyDescriptorSetLayout(context.getDevice(), descriptorSetLayouts[i], nullptr);
//		}
//	}
//
//private:
//
//	void createDescriptorPool(const GraphicsContext& context, int index, const MaterialInputSet& inputSet, int maxSetCount)
//	{
//		std::vector<VkDescriptorPoolSize> poolSizes(inputSet.size());
//		for (int i = 0; i < inputSet.size(); i++)
//		{
//			poolSizes[i].type = inputSet.getDescriptorType(i);
//			poolSizes[i].descriptorCount = 1;
//		}
//
//		VkDescriptorPoolCreateInfo poolInfo = {};
//		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
//		poolInfo.pPoolSizes = poolSizes.data();
//		poolInfo.maxSets = maxSetCount;
//
//		if (vkCreateDescriptorPool(context.getDevice(), &poolInfo, nullptr, &descriptorPools[index]) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create descriptor pool !");
//		}
//	}
//
//	void createDescriptorSetLayout(const GraphicsContext& context, int index, const MaterialInputSet& inputSet)
//	{
//		std::vector<VkDescriptorSetLayoutBinding> bindings;
//		inputSet.getDescriptorSetLayoutBindings(bindings);
//
//		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
//		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
//		layoutInfo.pBindings = bindings.data();
//
//		if (vkCreateDescriptorSetLayout(context.getDevice(), &layoutInfo, nullptr, &descriptorSetLayouts[index]) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create descriptor set layout !");
//		}
//	}
//
//	void allocateDescriptorSet(const GraphicsContext& context, size_t index)
//	{
//		VkDescriptorSetAllocateInfo allocInfo = {};
//		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//		allocInfo.descriptorPool = descriptorPools[index];
//		allocInfo.descriptorSetCount = 1;
//		allocInfo.pSetLayouts = &descriptorSetLayouts[index];
//
//		if (vkAllocateDescriptorSets(context.getDevice(), &allocInfo, &descriptorSets[index]) != VK_SUCCESS) {
//			throw std::runtime_error("failed to allocate descriptor set !");
//		}
//	}
//};