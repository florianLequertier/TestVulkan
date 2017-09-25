//#pragma once
//
//struct StaticMeshData
//{
//	std::vector<Vertex> vertex;
//	std::vector<uint32_t> index;
//
//	Buffer vertex;
//	Buffer index;
//};
//
//class StaticMesh
//{
//	StaticMeshData meshData;
//	AABB renderBox;
//};
//
//class Skeleton
//{
//	SkeletonData skeletonData;
//};
//
//class BoneTransform
//{
//	glm::vec3 pos;
//	glm::quat rot;
//};
//
//struct SkeletonData
//{
//	std::map<std::string, uint32_t> boneMappingNameToIdx;
//	std::vector<BoneTransform> boneBaseTransforms;
//	std::vector<uint32_t> boneChilds;
//};
//
//struct SkeletonInstanceData
//{
//	Skeleton* skeleton;
//	std::vector<BoneTransform> boneCurrentTransforms;
//};
//
//struct SkeletalMeshData
//{
//	std::vector<WeightedVertex> vertex;
//	std::vector<uint32_t> index;
//
//	Buffer vertex;
//	Buffer index;
//}
//
//class SkeletalMesh
//{
//	SkeletalMeshData meshData;
//	SkeletonInstanceData skeletonInstanceData;
//	AABB renderBox;
//};
//
//class MaterialInput
//{
//
//};
//
//// We create 3 pools to allocate 3 descriptor sets.
//// The global one handle global parameter of material (time, ...)
//// The local one handle parameter local to material instance ( most of textures, floats, ...)
//// The renderable one handle parameter describing renderable instance (transformations, ...)
//struct MaterialData
//{
//	Pipeline* pipelineRef;
//
//	DescriptorSetLayout setLayouts[3];
//	vkDescriptorSet descriptorSet[3];
//	VkWriteDescriptorSet descriptorSetWrite[3];
//	DescriptorPool descriptorPools[3];
//
//	MaterialInput materialGlobalInputs[];
//	MaterialInput materialLocalInputs[];
//	MaterialInput materialRenderableInputs[];
//
//	void init()
//	{
//		materialGlobalInputs.init();
//		materialLocalInputs.init();
//		materialRenderableInputs.init();
//
//		createDescriptorPool(materialGlobalInputs, setLayouts.size());
//		createDescriptorPool(materialLocalInputs, setLayouts.size());
//		createDescriptorPool(materialRenderableInputs, setLayouts.size());
//
//		createDescriptorSetLayout(materialGlobalInputs);
//		createDescriptorSetLayout(materialLocalInputs);
//		createDescriptorSetLayout(materialRenderableInputs);
//
//		updateAllDescriptorSetsWrite(materialGlobalInputs);
//		updateAllDescriptorSetsWrite(materialLocalInputs);
//		updateAllDescriptorSetsWrite(materialRenderableInputs);
//
//		allocateDescriptorSets(setLayouts);
//	}
//
//	void createDescriptorPool(MaterialInput& inputs, int maxSetCount)
//	{
//		std::vector<VkDescriptorPoolSize> poolSizes(inputs.size());
//		for ()
//		{
//			poolSizes[i].type = inputs.getDescriptorType();
//			poolSizes[i].descriptorCount = 1;
//		}
//
//		VkDescriptorPoolCreateInfo poolInfo = {};
//		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
//		poolInfo.pPoolSizes = poolSizes.data();
//		poolInfo.maxSets = maxSetCount;
//
//		if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create descriptor pool !");
//		}
//	}
//
//	void createDescriptorSetLayout(MaterialInput& inputs)
//	{
//		std::vector<VkDescriptorSetLayoutBinding> bindings(materialInputs.size());
//
//		for ()
//		{
//			bindings[i] = inputs[i]->getDescriptorSetLayoutBinding();
//		}
//
//		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
//		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
//		layoutInfo.pBindings = bindings.data();
//
//		if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create descriptor set layout !");
//		}
//	}
//
//	void updateAllDescriptorSetsWrite(MaterialInput& inputs)
//	{
//		std::vector<vkDescriptorSetWrite> descriptorSetsWrite;
//		
//		for ()
//		{
//			descriptorSetsWrite.push_back(inputs.descriptorSetsWrite[i]);
//		}
//		vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorSetsWrite.size()), descriptorSetsWrite.data(), 0, nullptr);
//	}
//
//	void allocateDescriptorSets(std::vector<DescriptorSetLayout>& layouts)
//	{
//		VkDescriptorSetAllocateInfo allocInfo = {};
//		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//		allocInfo.descriptorPool = descriptorPool;
//		allocInfo.descriptorSetCount = static_cast<uint32_t>layouts.size();
//		allocInfo.pSetLayouts = layouts.data();
//
//		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
//			throw std::runtime_error("failed to allocate descriptor set !");
//		}
//	}
//};
//
//
//class MaterialParameter
//{
//	string parameterDisplayName;
//
//	void DisplayInEditor();
//	{
//
//	}
//};
//
//class MaterialUniform : public MaterialParameter
//{
//	void* getValuePtr();
//	size_t getValueSize();
//};
//
//class MaterialUniformFloat : public MaterialUniform
//{
//	float value;
//};
//
//class MaterialUniformMat4 : public MaterialUniform
//{
//	glm::mat4 value;
//};
//
//class MaterialUniformBuffer : public MaterialInput
//{
//	std::vector<MaterialUniform*> uniforms;
//
//	Buffer ubo;
//
//	void AddUniform(MaterialUniform* newUniform)
//	{
//		uniforms.push_back(newUniform);
//	}
//
//	// create the ubo based on the uniforms
//	void init()
//	{
//		std::vector<void*> datas(ubo.size());
//
//		size_t totalSize = 0;
//		for (uniform : ubo)
//		{
//			totalSize += uniform->getValueSize();
//			datas[i] = ubo->getData();
//		}
//
//		// create buffer based on data size inside ubos
//		BufferCreateInfo createInfo;
//		createInfo.physicalDevice = physicalDevice;
//		createInfo.owningDevice = logicalDevice;
//		createInfo.itemCount = 1;
//		createInfo.itemSizeNotAligned = totalSize;
//		createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
//		uniformBuffer.create(createInfo, false);
//
//		// map these datas to buffer
//		BufferCopyInfo mappingInfo(totalSize, 1);
//		uniformBuffer.pushDatasToBuffer(physicalDevice, datas.data(), mappingInfo);
//	}
//
//	DescriptorSetWrite getDescriptorSetWrite()
//	{
//		VkDescriptorBufferInfo bufferInfo = {};
//		bufferInfo.buffer = *ubo.getBufferHandle();
//		bufferInfo.offset = 0;
//		bufferInfo.range = totalSize;
//
//		DescriptorSetWrite descriptorSetWrite;
//		descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//		descriptorSetWrite.dstSet = descriptorSet;
//		descriptorSetWrite.dstBinding = 0;
//		descriptorSetWrite.dstArrayElement = 0;
//		descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//		descriptorSetWrite.descriptorCount = 1;
//		descriptorSetWrite.pBufferInfo = &bufferInfo;
//
//		return descriptorSetWrite;
//	}
//};
//
//class ImageSampler : public MaterialInput, public MaterialParameter
//{
//	Texture2D* texture;
//
//	vkImage imageHandle;
//	vkSampler samplerHandle;
//
//	void setTexture(Texture2D* texture)
//	{
//		texture = texture;
//	}
//
//	void init()
//	{
//		imageHandle = texture->getImageHandle();
//		samplerHandle = texture->getSamplerHandle();
//	}
//
//	DescriptorSetWrite getDescriptorSetWrite()
//	{
//		VkDescriptorImageInfo imageInfo = {};
//		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//		imageInfo.sampler = textureSampler.getSamplerHandle();
//		imageInfo.imageView = textureImage.getImageViewHandle();
//
//		DescriptorSetWrite descriptorSetWrite;
//		descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//		descriptorSetWrite.dstSet = descriptorSet;
//		descriptorSetWrite.dstBinding = 1;
//		descriptorSetWrite.dstArrayElement = 0;
//		descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//		descriptorSetWrite.descriptorCount = 1;
//		descriptorSetWrite.pImageInfo = &imageInfo;
//
//		return descriptorSetWrite;
//	}
//};
//
//class MaterialInterface
//{
//	void use();
//	void bindGlobalUniforms();
//	void bindLocalUniforms();
//	void bindRenderableUniforms();
//};
//
//class Material : public MaterialInterface
//{
//	std::vector<MaterialInput> inputs;
//
//
//};
//
//class MaterialInstance : public MaterialInterface
//{
//
//};