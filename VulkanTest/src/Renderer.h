#pragma once

#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include <map>
#include <set>

#include "Buffer.h"
#include "GraphicsContext.h"
#include "Pipeline.h"
#include "WindowHandler.h"

class Material;
class MaterialInstance;
class MaterialInterface;
class RenderBatch;

// Datas representing a render pass. Includes datas for subPasses, framebuffer and sub pass dependencies

struct RenderPassData
{
	VkRenderPass renderPass;
	std::vector<VkSubpassDescription> subPasses;
	std::vector<std::vector<VkSubpassDependency>> subPassDependencies;
	std::vector<VkFramebuffer> frameBuffers;
	std::vector<std::shared_ptr<RenderBatch>> batchPerSubPasses;
};

// A render node encapsulate few commands and render passes
// Each node represents a single rendering feature
// (one for shadows, one for scene deferred rendering, one for tone mapping, one for bloom)
// Each render passes can be syncronized using semaphores
// To do this automatically, call addRenderPass() with indexes of previous passes to wait as second parameter
// If it is the first render pass of the node, give it a dependency of -1 to wait a previous node.

class RenderNode
{
protected:
	VkDevice owningDevice;
	VkCommandPool commandPool;

	// renderPasses for this node
	std::vector<RenderPassData> renderPasses;

	// command to draw the passes
	std::vector<VkCommandBuffer> commands;
	// semaphores to handle renderPass transitions
	std::vector<VkSemaphore> semaphores;
	// each index represent a signal semaphore to wait. We may have multiple semaphore to wait per pass.
	std::vector<std::vector<uint32_t>> waitSemaphoresPerPass;
	// semaphores to handle renderNodes transitions
	std::vector<VkSubmitInfo> submitInfos;

	std::multimap<uint32_t, uint32_t> queuedRenderPasses;

public:

	// usage
	void create(VkDevice device, VkCommandPool _commandPool)
	{
		owningDevice = device;
		commandPool = _commandPool;

		createCommands();

		createRenderPasses();
	}

	void addRenderPassNoDependencies(const RenderPassData& renderNode)
	{
		const size_t nodeIndex = renderNodes.size();
		renderNodes.push_back(renderNode);
	}

	void addRenderPass(const RenderPassData& renderPass, const std::vector<uint32_t>& dependencies)
	{
		const size_t passIndex = renderPasses.size();
		renderPasses.push_back(renderPass);
		makeNodeWaitOtherPasses(passIndex, dependencies);
	}

	void recordPrimaryCommands()
	{
		uint32_t passIndex = 0;
		for (const auto& renderPass : renderPasses)
		{
			VkCommandBufferBeginInfo commandBeginInfo = {};
			commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			commandBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			commandBeginInfo.pInheritanceInfo = nullptr;

			const VkCommandBuffer& commandBuffer = commands[passIndex].getCommandHandle();
			vkBeginCommandBuffer(commandBuffer, &commandBeginInfo);

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.clearValueCount;
			renderPassBeginInfo.pClearValues;
			renderPassBeginInfo.framebuffer = frameBuffers[passIndex];
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = ? ? ? ? ? ; //TODO
			renderPassBeginInfo.renderPass = renderPasses[passIndex].getRenderPassHandle();
			renderPassBeginInfo.clearValueCount;

			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			for (int subPassIndex = 0; subPassIndex < renderPasses[passIndex].subPasses.size(); subPassIndex++)
			{
				//const RenderBatch& batch = batchInfos.findBatchForPass(passIndex, subPassIndex).recordRenderCommand(commandBuffer);
				VkCommandBuffer secondaryCmdBuffers[] = { batch.getCommandBuffer() };
				vkCmdExecuteCommands(commandBuffer, 1, secondaryCmdBuffers);
				vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
			}
			vkCmdEndRenderPass(commandBuffer);

			vkEndCommandBuffer(commands.getCommandHandler());

			passIndex++;
		}
	}
	
	// call after all addRenderPass() calls
	// Will record commands and create submit infos
	void setup(const std::vector<VkSemaphore>& firstPassesWaitSemaphores)
	{
		setupSubmitInfos(firstPassesWaitSemaphores);
	}

	void recordSecondaryCommands()
	{
		uint32_t passIndex = 0;
		for (const auto& renderPassData : renderPasses)
		{
			for (int subPassIndex = 0; subPassIndex < renderPassData.subPasses.size(); subPassIndex++)
			{
				const RenderBatch& batch = renderPassData.batchPerSubPasses[subPassIndex];
				batch.recordRenderCommand(renderPassData.renderPass, subPassIndex);
			}
			passIndex++;
		}
	}

	void submitCommands(VkQueue graphicsQueue)
	{
		vkQueueSubmit(graphicsQueue, static_cast<uint32_t>(submitInfos.size()), submitInfos.data(), VK_NULL_HANDLE);
	}

	void destroy()
	{
		renderPasses.clear();

		waitSemaphoresPerPass.clear();
		submitInfos.clear();

		// clear semaphores
		for (int i = 0; i < semaphores.size(); i++)
			vkDestroySemaphore(owningDevice, semaphores[i], nullptr);

		// clear commands
		for (int i = 0; i < commands.size(); i++)
			vkFreeCommandBuffers(owningDevice, commandPool, static_cast<uint32_t>(commands.size()), commands.data());
	}

	// utility

	void makeNodeWaitOtherPasses(uint32_t waitingNode, const std::vector<uint32_t>& nodesToWait)
	{
		waitSemaphoresPerPass[waitingNode] = nodesToWait;
	}

	void extractLastSemaphores(std::vector<VkSemaphore>& outSemaphores)
	{
		// Special threatments for first queued commands because we need to wait previous render node
		// We extract we lasts semaphores of the previous node and set them to waitSemaphore field in submit info
		std::vector<uint32_t> lastPassesIndices;
		auto& lastRenderPassesIt = queuedRenderPasses.crbegin();
		const uint32_t& lastRenderPassesIndex = lastRenderPassesIt->first;
		for (; lastRenderPassesIt->first == lastRenderPassesIndex; lastRenderPassesIt++)
		{
			lastPassesIndices.push_back(lastRenderPassesIt->second);
		}

		outSemaphores.resize(lastPassesIndices.size());
		for (uint32_t semaphorePassIndex : lastPassesIndices)
		{
			outSemaphores.push_back(semaphores[semaphorePassIndex].getSemaphoreHandle());
		}
	}

	void setBatchForSubPass(uint32_t renderPassIndex, uint32_t subPassIndex, std::shared_ptr<RenderBatch>& renderBatch)
	{
		renderPasses[renderPassIndex].batchPerSubPasses[subPassIndex] = renderBatch;
	}

	void setBatchForAllSubPasses(std::shared_ptr<RenderBatch>& renderBatch)
	{
		for (auto& renderPass : renderPasses)
		{
			for (int subPassIndex = 0; subPassIndex < renderPass.subPasses.size(); subPassIndex++)
			{
				renderPass.batchPerSubPasses[subPassIndex] = renderBatch;
			}
		}
	}

	// getters

	virtual PipelineInfoSubpassRelated getPipelineInfoSubPassRelated(uint32_t renderPassIndex, uint32_t subPass) const
	{
		PipelineInfoSubpassRelated pipelineInfoSubpassRelated = {};
		pipelineInfoSubpassRelated.renderPass = renderPasses[renderPassIndex].renderPass;
		pipelineInfoSubpassRelated.subPass = subPass;
		pipelineInfoSubpassRelated.colorBlendingInfo = ;
		pipelineInfoSubpassRelated.depthStencil = ;
		pipelineInfoSubpassRelated.multisamplingInfo = ;
		pipelineInfoSubpassRelated.rasterizerInfo = ;
		pipelineInfoSubpassRelated.viewportState = ;
	}

	const RenderBatch& getBatch(uint32_t renderPassIndex, uint32_t subPassIndex) const
	{
		return *renderPasses[renderPassIndex].batchPerSubPasses[subPassIndex];
	}

	uint32_t getRenderPassCount() const
	{
		return renderPasses.size();
	}

	uint32_t getSubPassCount(uint32_t renderPassIndex) const
	{
		return renderPasses[renderPassIndex].subPasses.size();
	}

	VkRenderPass getRenderPass(uint32_t renderPassIndex) const
	{
		return renderPasses[renderPassIndex].renderPass;
	}

	VkRenderPass getSubPass(uint32_t renderPassIndex, uint32_t subPassIndex) const
	{
		return renderPasses[renderPassIndex].subPasses[subPassIndex];
	}

protected:
	virtual void createRenderPasses()
	{
		// nothing by default. Place here all the passes setup.
	}

private:

	void setupQueuedRenderPasses()
	{
		queuedRenderPasses.clear();

		int maxSemaphoreIndex = 0;
		int passIndex = 0;
		for (const auto& pass : renderPasses)
		{
			int localmaxSemaphoreIndex = 0;
			int passIndexOflocalmaxSemaphoreIndex = 0;
			const std::vector<uint32_t>& waitSemaphoreIndices = waitSemaphoresPerPass[passIndex];
			for (uint32_t semaphoreIndex : waitSemaphoreIndices)
			{
				if (semaphoreIndex > localmaxSemaphoreIndex)
				{
					localmaxSemaphoreIndex = semaphoreIndex;
					passIndexOflocalmaxSemaphoreIndex = passIndex;
				}
			}

			queuedRenderPasses.insert(std::make_pair(localmaxSemaphoreIndex + 1, passIndexOflocalmaxSemaphoreIndex));

			passIndex++;
		}
	}

	void createCommands()
	{
		commands.resize(renderPasses.size());

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandBufferCount = static_cast<uint32_t>(commands.size());
		allocateInfo.commandPool = commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		vkAllocateCommandBuffers(owningDevice, &allocateInfo, commands.data());
	}

	void setupSubmitInfos(const std::vector<VkSemaphore>& firstPassesWaitSemaphores)
	{
		submitInfos.resize(renderPasses.size());

		for (auto& it = queuedRenderPasses.cbegin(); it != queuedRenderPasses.end();)
		{
			uint32_t queueIndex = it->first;
			do
			{
				uint32_t passIndex = queuedRenderPasses.cbegin()->second;

				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				// wait semaphores
				if (queueIndex == 0)
				{
					// For the first queue, we wait the firstPassesWaitSemaphores
					VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
					submitInfo.waitSemaphoreCount = static_cast<uint32_t>(firstPassesWaitSemaphores.size());
					submitInfo.pWaitSemaphores = firstPassesWaitSemaphores.data();
					submitInfo.pWaitDstStageMask = waitStages;
				}
				else
				{
					const std::vector<uint32_t>& waitSemaphoreIndices = waitSemaphoresPerPass[passIndex];
					std::vector<VkSemaphore> waitSemaphores(waitSemaphoreIndices.size());
					for (uint32_t semaphoreIndex : waitSemaphoreIndices)
					{
						waitSemaphores.push_back(semaphores[semaphoreIndex].getSemaphoreHandle());
					}
					VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
					submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
					submitInfo.pWaitSemaphores = waitSemaphores.data();
					submitInfo.pWaitDstStageMask = waitStages;
				}

				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &(commands[passIndex].getCommandHandle());

				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = &(semaphores[passIndex].getSemaphoreHandle());

				submitInfos[passIndex] = submitInfo;

				++it;
			} while (it->first == queueIndex && it != queuedRenderPasses.end());
		}
	}

};

// A Render process represent the susseccion of multiple render nodes forming a coherent rendering
// Each node wait the previous node before it's execution.

class RenderProcess
{
private:
	// each node handle a rendering feature
	// (one for shadows, one for scene deferred rendering, one for tone mapping, one for bloom)
	std::vector<std::unique_ptr<RenderNode>> renderNodes;

public:

	//void create(VkDevice device, VkCommandPool commandPool)
	//{
	//	for (auto& renderNode : renderNodes)
	//	{
	//		renderNode->create(device, commandPool);
	//	}
	//}

	// override this to add you own nodes
	virtual void createNodes()
	{}

	void addRenderNode(std::unique_ptr<RenderNode>&& renderNode)
	{
		renderNodes.push_back(std::move(renderNode));
	}

	void setup(const std::vector<VkSemaphore>& firstPassesWaitSemaphores)
	{
		uint32_t nodeIndex = 0;
		for (auto& node : renderNodes)
		{
			std::vector<VkSemaphore> semaphores;
			if (nodeIndex > 0)
			{
				renderNodes[nodeIndex - 1]->extractLastSemaphores(semaphores);
			}
			else
			{
				semaphores = firstPassesWaitSemaphores;
			}

			node->setup(semaphores);

			nodeIndex++;
		}
	}

	void submitCommand(VkQueue graphicsQueue)
	{
		for (auto& node : renderNodes)
		{
			node->submitCommands(graphicsQueue);
		}
	}

	void destroy()
	{
		for (auto& renderNode : renderNodes)
		{
			renderNode->destroy();
		}
	}

	void extractLastSemaphores(std::vector<VkSemaphore>& outSemaphores)
	{
		renderNodes.back()->extractLastSemaphores(outSemaphores);
	}
};

struct RenderSetup
{
	bool validationLayersEnabled;
	std::vector<const char*> validationLayers;
	std::vector<const char*> deviceExtensions;
	VkPhysicalDeviceFeatures requiredDeviceFeatures;
	bool needPresentSupport = true;
	VkQueueFlags requestedQueueFlags = VK_QUEUE_GRAPHICS_BIT;
};

class Renderer
{

private:
	WindowHandler windowHandler;
	// the graphics API context (olding instance, physical device and device)
	GraphicsContext graphicsContext;
	WindowContext windowContext;
	RenderSetup renderSetup;

	// multiple graphical process to write in different render targets
	std::vector<std::unique_ptr<RenderProcess>> renderProcesses;

	VkSemaphore swapChainImageAvailableSemaphore;

public:
	Renderer()
	{
		renderSetup.validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
		renderSetup.deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		renderSetup.requiredDeviceFeatures = {};

		auto windowRequiredExtensions = windowHandler.getRequiredWindowExtensions();
		renderSetup.deviceExtensions.insert(renderSetup.deviceExtensions.end(), windowRequiredExtensions.begin(), windowRequiredExtensions.end());
		
		renderSetup.requiredDeviceFeatures.samplerAnisotropy = VK_TRUE;

		windowHandler.windowResizeCallback = [this](float width, float height) { this->onWindowResized(width, height); };
	}

	~Renderer()
	{

	}

	void onWindowResized(float width, float height)
	{
		//TODO
	}

	void create()
	{
		glm::vec2 initialWindowSize(800, 600);
		windowHandler.create(initialWindowSize, "Title");

		graphicsContext.createInstance(renderSetup);
		graphicsContext.setupDebugCallback(renderSetup);
		windowContext.createSurface(graphicsContext.getInstance(), *(windowHandler.getWindow()));
		graphicsContext.createPhysicalDevice(renderSetup, windowContext.getSurface());
		graphicsContext.initQueueFamilies(graphicsContext.getPhysicalDevice(), windowContext.getSurface());
		graphicsContext.createDevice(renderSetup);
		graphicsContext.createCommandPool();
		windowContext.createSwapChain(initialWindowSize, graphicsContext.getPhysicalDevice(), graphicsContext.getDevice(), graphicsContext.getQueueFamilies());
	}

	void destroy()
	{
		windowContext.destroy(graphicsContext.getInstance());
		graphicsContext.destroy();
		windowHandler.destroy();
	}

	// override this to add you own processes
	virtual void createProcesses()
	{}

	void destroyProcesses()
	{
		for (auto& process : renderProcesses)
		{
			process->destroy();
		}

		renderProcesses.clear();
	}

	// call setup() on all processes
	void setupProcesses()
	{
		for (auto& process : renderProcesses)
		{
			process->setup();
		}
	}

	// record all processes commands
	void recordCommands()
	{
		for (auto& process : renderProcesses)
		{
			process->recordPrimaryCommands();
		}
	}

	// submit all process commands
	void submitProcesses()
	{
		// acquire image
		if (renderSetup.validationLayersEnabled)
			vkDeviceWaitIdle(graphicsContext.getDevice());

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(graphicsContext.getDevice(), windowContext.getSwapChain(), std::numeric_limits<uint64_t>::max(), swapChainImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image !");
		}

		std::vector<VkSemaphore> presentWaitSemaphores;
		// submit all processes
		for (auto& process : renderProcesses)
		{
			process->submitCommands(graphicsContext.getGraphicsQueue());

			std::vector<VkSemaphore> lastSemaphores;
			process->extractLastSemaphores(lastSemaphores);
			presentWaitSemaphores.insert(presentWaitSemaphores.end(), lastSemaphores.begin(), lastSemaphores.end());
		}

		// present image
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = static_cast<uint32_t>(presentWaitSemaphores.size());
		presentInfo.pWaitSemaphores = presentWaitSemaphores.data();
		presentInfo.swapchainCount = 1;
		VkSwapchainKHR swapChain[] = { windowContext.getSwapChain() };
		presentInfo.pSwapchains = swapChain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; //Optional
		VkQueue presentQueue = graphicsContext.getPresentQueue();
		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image !");
		}
	}

	std::vector<const char*> getRequiredExtensions() const
	{
		std::vector<const char*> extensions;

		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		for (unsigned int i = 0; i < glfwExtensionCount; i++)
		{
			extensions.push_back(glfwExtensions[i]);
		}

		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}

		return extensions;
	}

	bool getValidationLayersEnabled() const
	{
		return validationLayersEnabled;
	}

	const std::vector<const char*>& getValidationLayers() const
	{
		return validationLayers;
	}

	const std::vector<const char*>& getDeviceExtensions() const
	{
		return deviceExtensions;
	}

	void printInfosToConsole() const
	{
		// print extensions' name
		{
			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> extensions(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
			std::cout << "availlable extensions : " << std::endl;
			for (const auto& extension : extensions) {
				std::cout << "\t" << extension.extensionName << std::endl;
			}
		}
	}

	const GraphicsContext& getGraphicsContext() const
	{
		return graphicsContext;
	}

	const WindowContext& getWindowContext() const
	{
		return windowContext;
	}
};
