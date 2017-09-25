#pragma once

#include <vulkan/vulkan.hpp>


class Sampler final
{
private:

	VkDevice owningDevice;
	VkSampler sampler;

public:

	Sampler()
		: owningDevice(VK_NULL_HANDLE)
		, sampler(VK_NULL_HANDLE)
	{

	}

	Sampler(VkDevice device, const VkSamplerCreateInfo* info)
	{
		create(device, info);
	}

	~Sampler()
	{
		destroy();
	}

	void create(VkDevice device, const VkSamplerCreateInfo* info)
	{
		owningDevice = device;

		if (vkCreateSampler(owningDevice, info, nullptr, &sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create sampler !");
		}
	}

	void destroy()
	{
		if (owningDevice != VK_NULL_HANDLE && sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(owningDevice, sampler, nullptr);
			owningDevice == VK_NULL_HANDLE;
			sampler == VK_NULL_HANDLE;
		}
	}

	VkSampler getSamplerHandle() const
	{
		return sampler;
	}
};