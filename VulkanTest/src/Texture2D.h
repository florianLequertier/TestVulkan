#pragma once

#include "MaterialParameter.h"
#include "MaterialInputs.h"

#include "Image.h"
#include "Sampler.h"

class Texture2D : public MaterialInput, public MaterialParameter
{
	Image2D image;
	Sampler sampler;

	Texture2D()
	{}

	~Texture2D()
	{}

	void create(const Image2DCreateInfo& imageCreateInfo, const VkSamplerCreateInfo* samplerCreateInfo)
	{
		image.create(imageCreateInfo);
		sampler.create(imageCreateInfo.device, samplerCreateInfo);
	}

	void destroy()
	{
		image.destroy();
		sampler.destroy();
	}


	VkWriteDescriptorSet getWriteDescriptorSet(const VkDescriptorSet& owningSet) const
	{
		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageView = image.getImageViewHandle();
		imageInfo.imageLayout = image.getLayout();
		imageInfo.sampler = sampler.getSamplerHandle();

		VkWriteDescriptorSet descriptorSetWrite;
		descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrite.dstSet = owningSet;
		descriptorSetWrite.dstBinding = 0;
		descriptorSetWrite.dstArrayElement = 0;
		descriptorSetWrite.descriptorType = getDescriptorType();
		descriptorSetWrite.descriptorCount = 1;
		descriptorSetWrite.pImageInfo = &imageInfo;

		return descriptorSetWrite;
	}

	VkDescriptorType getDescriptorType() const
	{
		return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	}

	const VkDescriptorSetLayoutBinding& getDescriptorSetLayoutBinding() const
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