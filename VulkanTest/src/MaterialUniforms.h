#pragma once

#include "MaterialParameter.h"
#include "MaterialInputs.h"

class GraphicsContext;

// MaterialUniformBuffer represents a material input for uniform buffer object

class MaterialInternalUniformBase : public MaterialParameter
{
public:
	virtual void* getValuePtr() = 0;
	virtual size_t getValueSize() = 0;
};

template<typename T>
class MaterialInternalUniform : public MaterialUniformBase
{
private:
	T value;

public:
	virtual void* getValuePtr() override
	{
		return &value;
	}

	virtual size_t getValueSize() override
	{
		return sizeof(T);
	}
};

class IMaterialInternalParameterBuffer
{
public:
	virtual void createGPUSide(const GraphicsContext& context) = 0;
	virtual void destroyGPUSide(const VkDevice& device) = 0;
};

class MaterialInternalUniformParameterBuffer : public IMaterialInternalParameterBuffer
{
private:
	std::vector<std::unique_ptr<MaterialInternalUniformBase>> uniforms;
	Buffer ubo;

public:
	void AddUniform(std::unique_ptr<MaterialInternalUniformBase>&& newUniform)
	{
		uniforms.push_back(std::move(newUniform));
	}

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
		BufferCreateInfo createInfo;
		createInfo.physicalDevice = context.getPhysicalDevice();
		createInfo.owningDevice = context.getDevice();
		createInfo.itemCount = 1;
		createInfo.itemSizeNotAligned = totalSize;
		createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		ubo.create(createInfo, false);

		// map these datas to buffer
		BufferCopyInfo mappingInfo(totalSize, 1);
		ubo.pushDatasToBuffer(context.getPhysicalDevice(), datas.data(), mappingInfo);
	}

	void destroyGPUSide(const VkDevice& device) override
	{
		ubo.destroy();
	}
};

class MaterialUniformBuffer final : public MaterialInput
{
private:
	Buffer* ubo;

public:
	void setUBORef(Buffer* uboRef)
	{
		ubo = uboRef;
	}

	VkWriteDescriptorSet getWriteDescriptorSet(const VkDescriptorSet& owningSet) const override
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = *(ubo->getBufferHandle());
		bufferInfo.offset = 0;
		bufferInfo.range = ubo->getSize();

		VkWriteDescriptorSet descriptorSetWrite;
		descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrite.dstSet = owningSet;
		descriptorSetWrite.dstBinding = binding;
		descriptorSetWrite.dstArrayElement = 0;
		descriptorSetWrite.descriptorType = getDescriptorType();
		descriptorSetWrite.descriptorCount = 1;
		descriptorSetWrite.pBufferInfo = &bufferInfo;

		return descriptorSetWrite;
	}

	VkDescriptorType getDescriptorType() const override
	{
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}

	const VkDescriptorSetLayoutBinding& getDescriptorSetLayoutBinding() const override
	{
		VkDescriptorSetLayoutBinding binding;
		binding.binding = binding;
		binding.descriptorCount = 1;
		binding.descriptorType = getDescriptorType();
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		return binding;
	}
};