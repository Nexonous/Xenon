// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "VulkanCommandSubmitter.hpp"
#include "VulkanMacros.hpp"
#include "VulkanCommandRecorder.hpp"
#include "VulkanSwapchain.hpp"

#include <optick.h>

namespace Xenon
{
	namespace Backend
	{
		VulkanCommandSubmitter::VulkanCommandSubmitter(VulkanDevice* pDevice)
			: CommandSubmitter(pDevice)
			, VulkanDeviceBoundObject(pDevice)
		{
			// Create the fence.
			VkFenceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			createInfo.flags = 0;
			createInfo.pNext = nullptr;

			XENON_VK_ASSERT(m_pDevice->getDeviceTable().vkCreateFence(m_pDevice->getLogicalDevice(), &createInfo, nullptr, &m_WaitFence), "Failed to create fence!");
		}

		VulkanCommandSubmitter::~VulkanCommandSubmitter()
		{
			try
			{
				wait();
			}
			catch (...)
			{
				XENON_LOG_ERROR("Failed to wait for the command submitter to finish execution!");
			}

			m_pDevice->getDeviceTable().vkDestroyFence(m_pDevice->getLogicalDevice(), m_WaitFence, nullptr);
		}

		void VulkanCommandSubmitter::submit(const std::vector<std::vector<Backend::CommandRecorder*>>& pCommandRecorders, Swapchain* pSwapchain /*= nullptr*/)
		{
			OPTICK_EVENT();

			std::vector<VkSubmitInfo> submitInfos;
			std::vector<std::vector<VkCommandBuffer>> commandBuffers;
			std::vector<std::vector<VkPipelineStageFlags>> waitStageFlags;
			std::vector<std::vector<VkSemaphore>> waitSemaphores;
			std::vector<std::vector<VkSemaphore>> signalSemaphores;

			submitInfos.reserve(pCommandRecorders.size());
			commandBuffers.reserve(pCommandRecorders.size());
			waitSemaphores.reserve(pCommandRecorders.size() + (pSwapchain ? 1 : 0));
			waitStageFlags.reserve(pCommandRecorders.size() + (pSwapchain ? 1 : 0));
			signalSemaphores.reserve(pCommandRecorders.size() + (pSwapchain ? 1 : 0));

			auto pVkSwapchain = pSwapchain->as<VulkanSwapchain>();
			if (pVkSwapchain != nullptr && pVkSwapchain->isRenderable())
			{
				waitSemaphores.emplace_back().emplace_back(*pVkSwapchain->getInFlightSemaphorePtr());
				waitStageFlags.emplace_back().emplace_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			}

			m_bIsWaiting = !pCommandRecorders.empty();

			// Create the submit info structure.
			for (const auto& pCommandRecorderBatch : pCommandRecorders)
			{
				auto& batchCommandBuffers = commandBuffers.emplace_back();
				batchCommandBuffers.reserve(pCommandRecorderBatch.size());

				auto& batchSignalSemaphores = signalSemaphores.emplace_back();
				batchSignalSemaphores.reserve(pCommandRecorderBatch.size());

				std::vector<VkPipelineStageFlags> waitFlags;
				waitFlags.reserve(pCommandRecorderBatch.size());

				auto& submitInfo = submitInfos.emplace_back();
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				for (const auto pCommandRecorder : pCommandRecorderBatch)
				{
					auto pVkCommandBuffer = pCommandRecorder->as<VulkanCommandRecorder>()->getCurrentCommandBuffer();
					batchCommandBuffers.emplace_back(*pVkCommandBuffer);
					batchSignalSemaphores.emplace_back(pVkCommandBuffer->getSignalSemaphore());
					waitFlags.emplace_back(pVkCommandBuffer->getStageFlags());

					if (!waitSemaphores.empty())
					{
						const auto& semaphores = waitSemaphores.back();
						const auto& stageFlags = waitStageFlags.back();

						submitInfo.waitSemaphoreCount = static_cast<uint32_t>(semaphores.size());
						submitInfo.pWaitSemaphores = semaphores.data();
						submitInfo.pWaitDstStageMask = stageFlags.data();
					}
				}

				submitInfo.commandBufferCount = static_cast<uint32_t>(pCommandRecorderBatch.size());
				submitInfo.pCommandBuffers = batchCommandBuffers.data();
				submitInfo.signalSemaphoreCount = static_cast<uint32_t>(batchSignalSemaphores.size());
				submitInfo.pSignalSemaphores = batchSignalSemaphores.data();

				waitStageFlags.emplace_back(std::move(waitFlags));
				waitSemaphores.emplace_back(batchSignalSemaphores);
			}

			// Get the semaphores from the swapchain if provided.
			if (!submitInfos.empty() && pVkSwapchain != nullptr && pVkSwapchain->isRenderable())
			{
				auto& submitInfo = submitInfos.back();
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = pVkSwapchain->getRenderFinishedSemaphorePtr();
			}

			// Submit the queue.
			m_pDevice->getGraphicsQueue().access([this, submitInfos = std::move(submitInfos)](const VulkanQueue& queue)
				{
					XENON_VK_ASSERT(m_pDevice->getDeviceTable().vkQueueSubmit(queue.getQueue(), static_cast<uint32_t>(submitInfos.size()), submitInfos.data(), m_WaitFence), "Failed to submit the queue!");
				}
			);
		}

		void VulkanCommandSubmitter::wait(std::chrono::milliseconds timeout /*= std::chrono::milliseconds(UINT64_MAX)*/)
		{
			OPTICK_EVENT();

			if (m_bIsWaiting)
			{
				XENON_VK_ASSERT(m_pDevice->getDeviceTable().vkWaitForFences(m_pDevice->getLogicalDevice(), 1, &m_WaitFence, VK_TRUE, timeout.count()), "Failed to wait for the fence!");
				XENON_VK_ASSERT(m_pDevice->getDeviceTable().vkResetFences(m_pDevice->getLogicalDevice(), 1, &m_WaitFence), "Failed to reset fence!");
				m_bIsWaiting = false;
			}
		}
	}
}