#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <map>

#include "Buffer.h"
#include "Renderable.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescription = {};
		attributeDescription[0].binding = 0;
		attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[0].location = 0;
		attributeDescription[0].offset = offsetof(Vertex, position);

		attributeDescription[1].binding = 0;
		attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[1].location = 1;
		attributeDescription[1].offset = offsetof(Vertex, color);

		attributeDescription[2].binding = 0;
		attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;;
		attributeDescription[2].location = 2;
		attributeDescription[2].offset = offsetof(Vertex, texCoord);

		return attributeDescription;
	}
};


template<typename VertexType>
class TMeshData
{
private:
	std::vector<VertexType> vertices;
	std::vector<uint32_t> indices;

	Buffer vertexBuffer;
	Buffer indexBuffer;

public:
	void setVertexCount(size_t count)
	{
		vertices.resize(count);
	}

	bool setVertexData(uint32_t vertexIndex, const VertexType& data, bool adaptSize = false)
	{
		if (vertexIndex < 0 || (!adaptSize && vertexIndex >= vertices.size()))
			return false;

		if (adaptSize && vertexIndex >= vertices.size())
			vertices.resize(vertexIndex + 1);

		vertices[vertexIndex] = data;
		return true;
	}

	bool setVerticesData(const std::vector<VertexType>& verticesData, uint32_t firstIndex = 0, bool adaptSize = false)
	{
		if (adaptSize)
			setVertexCount(firstIndex + verticesData.size());

		int index = firstIndex;
		bool success = true;
		for (const auto& vertexData : verticesData)
		{
			success = setVertexData(index, vertexData, false);
			if (!success)
				return false;
		}
		return success;
	}

	void createGPUSide(const GraphicsContext& context)
	{
		{
			BufferCreateInfo createInfo = {};
			createInfo.itemCount = static_cast<uint32_t>(vertexBuffer.getSize());
			createInfo.itemSizeNotAligned = sizeof(VertexType);
			createInfo.owningDevice = context.getDevice();
			createInfo.physicalDevice = context.getPhysicalDevice();
			createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

			vertexBuffer.create(createInfo, true);
		}

		{
			BufferCreateInfo createInfo = {};
			createInfo.itemCount = static_cast<uint32_t>(indexBuffer.getSize());
			createInfo.itemSizeNotAligned = sizeof(uint32_t);
			createInfo.owningDevice = context.getDevice();
			createInfo.physicalDevice = context.getPhysicalDevice();
			createInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

			indexBuffer.create(createInfo, true);
		}
	}

	void destroyGPUSide()
	{
		vertexBuffer.destroy();
		indexBuffer.destroy();
	}

	const Buffer& getVertexBuffer() const
	{
		return vertexBuffer;
	}

	const Buffer& getIndexBuffer() const
	{
		return indexBuffer;
	}
};

typedef TMeshData<Vertex> StaticMeshData;
typedef TMeshData<WeightedVertex> SkeletalMeshData;

class StaticMesh : public Renderable
{
private:
	StaticMeshData meshData;
	//AABB renderBox;

public:
	void createGPUSide(const GraphicsContext& context)
	{
		meshData.createGPUSide(context);
	}

	void destroyGPUSide()
	{
		meshData.destroyGPUSide();
	}

	void cmdbindVBOsAndIBOs(VkCommandBuffer commandBuffer) override
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, meshData.getVertexBuffer().getBufferHandle(), offsets);
		vkCmdBindIndexBuffer(commandBuffer, *meshData.getIndexBuffer().getBufferHandle(), 0, VK_INDEX_TYPE_UINT16);
	}
	virtual void cmdDraw(VkCommandBuffer commandBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, meshData.getIndexBuffer().getItemCount(), 1, 0, 0, 0);
	}
};

class SkeletalMesh : public Renderable
{
	SkeletalMeshData meshData;
	SkeletonInstanceData skeletonInstanceData;
	//AABB renderBox;

public:
	void createGPUSide(const GraphicsContext& context)
	{
		meshData.createGPUSide(context);
	}

	void destroyGPUSide()
	{
		meshData.destroyGPUSide();
	}

	void cmdbindVBOsAndIBOs(VkCommandBuffer commandBuffer) override
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, meshData.getVertexBuffer().getBufferHandle(), offsets);
		vkCmdBindIndexBuffer(commandBuffer, *meshData.getIndexBuffer().getBufferHandle(), 0, VK_INDEX_TYPE_UINT16);
	}
	virtual void cmdDraw(VkCommandBuffer commandBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, meshData.getIndexBuffer().getItemCount(), 1, 0, 0, 0);
	}
};

template<typename KeyType>
struct AnimKey
{
	KeyType value;
	float time;
};

class SkeletalAnimation
{
private:
	float duration;
	float ticksPerSecond;

	std::vector<float> rotationKeysTime;
	std::vector<std::vector<glm::quat>> bonesRotation;
	std::vector<float> translationKeysTime;
	std::vector<std::vector<glm::vec3>> bonesTranslation;

public:
	uint32_t getRotationKeyIndex(float animationTime, float& outOverTime) const
	{
		for (uint32_t keyIndex = 0; keyIndex  < rotationKeysTime.size(); keyIndex++)
		{
			// reach last key : we can't go further
			if (keyIndex == rotationKeysTime.size() - 1)
			{
				outOverTime = animationTime - rotationKeysTime.back();
				return keyIndex;
			}

			if (rotationKeysTime[keyIndex] <= animationTime
				&& rotationKeysTime[keyIndex + 1] > animationTime)
			{
				outOverTime = animationTime - rotationKeysTime[keyIndex];
				return keyIndex;
			}
		}
	}

	uint32_t getTranslationKeyIndex(float animationTime, float& outOverTime) const
	{
		for (uint32_t keyIndex = 0; keyIndex < translationKeysTime.size(); keyIndex++)
		{
			// reach last key : we can't go further
			if (keyIndex == translationKeysTime.size() - 1)
			{
				outOverTime = animationTime - translationKeysTime.back();
				return keyIndex;
			}

			if (translationKeysTime[keyIndex] <= animationTime
				&& translationKeysTime[keyIndex + 1] > animationTime)
			{
				outOverTime = animationTime - translationKeysTime[keyIndex];
				return keyIndex;
			}
		}
	}

	const glm::quat& getRotation(uint32_t boneIndex, uint32_t rotationKeyIndex) const
	{
		return bonesRotation[boneIndex][rotationKeyIndex];
	}

	const glm::vec3& getTranslation(uint32_t boneIndex, uint32_t translationKeyIndex) const
	{
		return bonesTranslation[boneIndex][translationKeyIndex];
	}

	float getTickPerSecond() const
	{
		return ticksPerSecond;
	}
};

class SkeletalAnimationInstance
{
private:
	SkeletalAnimation* animation;
	float animationBeginTime;

public:
	float getAnimationTickTime(float timeInSecond) const
	{
		return timeInSecond * animation->getTickPerSecond();
	}

	uint32_t getRotationKeyIndex(float animationTime, float& outOverTime) const
	{
		return animation->getRotationKeyIndex(animationTime, outOverTime);
	}

	uint32_t getTranslationKeyIndex(float animationTime, float& outOverTime) const
	{
		return animation->getTranslationKeyIndex(animationTime, outOverTime);
	}

	const glm::quat& getRotation(uint32_t boneIndex, uint32_t rotationKeyIndex) const
	{
		return animation->getRotation(boneIndex, rotationKeyIndex);
	}

	const glm::vec3& getTranslation(uint32_t boneIndex, uint32_t translationKeyIndex) const
	{
		return animation->getTranslation(boneIndex, translationKeyIndex);
	}

	float getTickPerSecond() const
	{
		return animation->getTickPerSecond();
	}
};

class Skeleton
{
	SkeletonData skeletonData;

	static void computeAnimationStep(const SkeletonInstanceData& skeletonInstance, float time, const SkeletalAnimationInstance& animation)
	{
		float animTickTime = animation.getAnimationTickTime(time);

		float rotationOverTime = 0;
		int rotationKeyIndex = animation.getRotationKeyIndex(animTickTime, rotationOverTime);
		float translationOverTime = 0;
		int translationKeyIndex = animation.getTranslationKeyIndex(animTickTime, translationOverTime);
		for (int i = 0; i < skeletonInstance.boneCurrentTransforms.size(); i++)
		{
			skeleton.boneCurrentTransforms[i].position = BoneTransform::interpolateRotation(animation.getRotation(i, rotationKeyIndex), animation.getRotation(i, rotationKeyIndex + 1));
			skeleton.boneCurrentTransforms[i].rotation = BoneTransform::interpolateTranslation(animation.getTranslation(i, translationKeyIndex), animation.getTranslation(i, translationKeyIndex + 1));
			skeleton.boneCurrentTransforms[i] = skeletonInstance.getRootInverseTransform() * skeleton.boneCurrentTransforms[i] * skeletonInstance.getBoneBaseTransform(i);
		}
	}
};

struct BoneTransform
{
	glm::vec3 position;
	glm::quat rotation;
};

struct SkeletonData
{
	glm::mat4 rootInverseTransform;
	std::map<std::string, uint32_t> boneMappingNameToIdx;
	std::vector<BoneTransform> boneBaseTransforms;
	std::vector<uint32_t> boneChilds;
};

struct SkeletonInstanceData
{
	SkeletonData* skeletonData;
	std::vector<BoneTransform> boneCurrentTransforms;

	const glm::mat4& getRootInverseTransform() const
	{
		return skeletonData->rootInverseTransform;
	}

	const BoneTransform& getBoneBaseTransform(uint32_t boneIndex)
	{
		return skeletonData->boneBaseTransforms[boneIndex];
	}
};

struct WeightedVertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::ivec4 boneIndices;
	glm::vec4 weights;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(WeightedVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescription = {};
		attributeDescription[0].binding = 0;
		attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[0].location = 0;
		attributeDescription[0].offset = offsetof(Vertex, position);

		attributeDescription[1].binding = 0;
		attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescription[1].location = 1;
		attributeDescription[1].offset = offsetof(Vertex, color);

		attributeDescription[2].binding = 0;
		attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription[2].location = 2;
		attributeDescription[2].offset = offsetof(Vertex, texCoord);

		attributeDescription[3].binding = 0;
		attributeDescription[3].format = VK_FORMAT_R32G32B32A32_SINT;
		attributeDescription[3].location = 3;
		attributeDescription[3].offset = offsetof(WeightedVertex, boneIndices);

		attributeDescription[4].binding = 0;
		attributeDescription[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescription[4].location = 4;
		attributeDescription[4].offset = offsetof(WeightedVertex, weights);

		return attributeDescription;
	}
};
