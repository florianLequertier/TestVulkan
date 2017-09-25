#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "Buffer.h"

// Material input and MaterialInputSet only handle the allocation of the resource (buffer/sampler/...)
// The allocation of descriptors is performed inside MaterialDescriptors

// Base class for representing an input in a shader
// Can be for example a ubo or a texture sampler
// The creation and deletion are implemented in derived class
class MaterialInput
{
protected:
	uint32_t binding;

public:

	MaterialInput( uint32_t _binding)
		: binding(_binding)
	{}

	virtual VkWriteDescriptorSet getWriteDescriptorSet(const VkDescriptorSet& owningSet) const = 0;
	virtual VkDescriptorType getDescriptorType() const = 0;
	virtual const VkDescriptorSetLayoutBinding& getDescriptorSetLayoutBinding() const = 0;
	uint32_t getBinding() const
	{
		return binding;
	}
};


// Represent a set of inputs
class MaterialInputSet
{
private:
	std::vector<std::unique_ptr<MaterialInput>> inputs;
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

public:

	template<template InputClass>
	void addInput()
	{
		const size_t newInputIndex = inputs.size();
		inputs.push_back(std::make_unique<InputClass>(this, newInputIndex));
	}

	void createGPUSide(const GraphicsContext& context, VkDescriptorPool descriptorPool)
	{
		createDescriptorSetLayout(context);
		allocateDescriptorSet(context, descriptorPool);

		assert(inputs.size() == writeDescriptorSets.size());

		int index = 0;
		for (auto input : inputs)
		{
			writeDescriptorSets[index] = input->getWriteDescriptorSet(descriptorSet);
			index++;
		}
	}

	void createDescriptorSetLayout(const GraphicsContext& context)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		getDescriptorSetLayoutBindings(bindings);

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(context.getDevice(), &layoutInfo, nullptr, &descriptorSetLayouts[index]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout !");
		}
	}

	void allocateDescriptorSet(const GraphicsContext& context, VkDescriptorPool descriptorPool)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPools;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(context.getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor set !");
		}
	}

	void destroyGPUSide(const VkDevice& device, VkDescriptorPool descriptorPool)
	{
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkFreeDescriptorSets(device, descriptorPool, , );
	}

	void getDescriptorSetLayoutBindings(std::vector<VkDescriptorSetLayoutBinding>& outBindings) const
	{
		outBindings.resize(inputs.size());

		for (size_t i = 0; i < inputs.size(); i++)
		{
			outBindings[i] = inputs[i]->getDescriptorSetLayoutBinding();
		}
	}

	void updateAllWriteDescriptorSets(const GraphicsContext& context)
	{
		vkUpdateDescriptorSets(context.getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void updateWriteDescriptorSet(const GraphicsContext& context, int inputIndex)
	{
		vkUpdateDescriptorSets(context.getDevice(), 1, &writeDescriptorSets[inputIndex], 0, nullptr);
	}

	void appendPoolSizes(std::vector<VkDescriptorPoolSize>& outSizes)
	{
		for (int i = 0; i < inputs.size(); i++)
		{
			outSizes.push_back(VkDescriptorPoolSize{ inputs[i]->getDescriptorType(), 1 });
		}
	}

	// getters

	VkDescriptorSetLayout getDescriptorSetLayout() const
	{
		return descriptorSetLayout;
	}

	VkDescriptorSet getDescriptorSet() const
	{
		return descriptorSet;
	}

	size_t size() const
	{
		return inputs.size();
	}

	VkWriteDescriptorSet getWriteDescriptorSet(int inputIndex, const VkDescriptorSet& owningSet) const
	{
		return inputs[inputIndex]->getWriteDescriptorSet(owningSet);
	}

	VkDescriptorType getDescriptorType(int inputIndex) const
	{
		return inputs[inputIndex]->getDescriptorType();
	}

	void getPoolSizes(std::vector<VkDescriptorPoolSize>& outSizes) const
	{
		outSizes.resize(inputs.size());
		for (int i = 0; i < inputs.size(); i++)
		{
			outSizes[i] = (VkDescriptorPoolSize{ inputs[i]->getDescriptorType(), 1 });
		}
	}

};

// Inputs specific to renderables
class StaticMeshMaterialInput : public MaterialInput
{
private:
	// The buffer must contains all transforms of static mesh into a given per material instance slot in batch
	Buffer allTransforms;

public:

	// create the ubo based on the uniforms
	void createGPUSide(const GraphicsContext& context) override
	{
		std::vector<void*> datas(uniforms.size());

		size_t totalSize = 0;
		int index = 0;
		for (auto& uniform : uniforms)
		{
			totalSize += uniform->getValueSize();
			datas[index] = uniform->getValuePtr();
			index++;
		}

		// create buffer based on data size inside ubos
		BufferCreateInfo createInfo = BufferCreateInfo::MakeFromNotAligned(context.getPhysicalDevice()
			, context.getDevice()
			, 1
			, totalSize
			, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
		);
		allTransforms.create(createInfo, false);

		// map these datas to buffer
		BufferCopyInfo mappingInfo(totalSize, 1);
		ubo.pushDatasToBuffer(context.getPhysicalDevice(), datas.data(), mappingInfo);
	}

	virtual void destroyGPUSide(const GraphicsContext& context) override
	{
		ubo.destroy();
	}

	VkWriteDescriptorSet getWriteDescriptorSet(const VkDescriptorSet& owningSet) const override
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = *ubo.getBufferHandle();
		bufferInfo.offset = 0;
		bufferInfo.range = ubo.getSize();

		VkWriteDescriptorSet descriptorSetWrite;
		descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrite.dstSet = owningSet;
		descriptorSetWrite.dstBinding = 0;
		descriptorSetWrite.dstArrayElement = 0;
		descriptorSetWrite.descriptorType = getDescriptorType();
		descriptorSetWrite.descriptorCount = 1;
		descriptorSetWrite.pBufferInfo = &bufferInfo;

		return descriptorSetWrite;
	}

	VkDescriptorType getDescriptorType() const override
	{
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	}

	const VkDescriptorSetLayoutBinding& getDescriptorSetLayoutBinding() const override
	{
		VkDescriptorSetLayoutBinding binding;
		binding.binding = 0;
		binding.descriptorCount = 1;
		binding.descriptorType = getDescriptorType();
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		return binding;
	}

};