#include "MaterialUniforms.h"

#include "GraphicsContext.h"

void MaterialUniformBuffer::AddUniform(std::unique_ptr<MaterialUniformBase> newUniform)
{
	uniforms.push_back(newUniform);
}

// create the ubo based on the uniforms
void MaterialUniformBuffer::createGPUSide(const GraphicsContext& context)
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

void MaterialUniformBuffer::destroyGPUSide(const GraphicsContext& context)
{
	ubo.destroy();
}

VkWriteDescriptorSet MaterialUniformBuffer::getWriteDescriptorSet(const VkDescriptorSet& owningSet) const
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
	descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetWrite.descriptorCount = 1;
	descriptorSetWrite.pBufferInfo = &bufferInfo;

	return descriptorSetWrite;
}

VkDescriptorType MaterialUniformBuffer::getDescriptorType() const
{
	return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}

const VkDescriptorSetLayoutBinding& MaterialUniformBuffer::getDescriptorSetLayoutBinding() const
{
	VkDescriptorSetLayoutBinding binding;
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	return binding;
}