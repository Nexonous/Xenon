// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VulkanDeviceBoundObject.hpp"

#ifdef max
#undef max

#endif

namespace Xenon
{
	namespace Backend
	{
		class VulkanSwapchain;

		/**
		 * Vulkan command buffer structure.
		 * This contains the actual Vulkan command buffer and it's synchronization primitives.
		 */
		class VulkanCommandBuffer final : public VulkanDeviceBoundObject
		{
		public:
			/**
			 * Explicit constructor.
			 *
			 * @param pDevice The device pointer to which the command buffer belongs to.
			 * @param buffer The allocated command buffer.
			 * @param commandPool The command pool to allocate the command buffer from.
			 * @param stageFlags The command buffer's pipeline stage flags.
			 */
			explicit VulkanCommandBuffer(VulkanDevice* pDevice, VkCommandBuffer buffer, VkCommandPool commandPool, VkPipelineStageFlags stageFlags);

			/**
			 * Move constructor.
			 *
			 * @param other The other command buffer.
			 */
			VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept;

			/**
			 * Destructor.
			 */
			~VulkanCommandBuffer() override;

			/**
			 * This will block the thread and wait till the command buffer has finished it's execution.
			 *
			 * @param timeout The timeout time to wait for. Default is the uint64_t max.
			 */
			void wait(uint64_t timeout = std::numeric_limits<uint64_t>::max());

			/**
			 * Submit the command buffer to the device.
			 *
			 * @param pipelineStageFlags The pipeline stage flags.
			 * @param queue The queue to submit to.
			 * @param pSwapchain The swapchain to get the semaphores from. Default is nullptr.
			 */
			void submit(VkPipelineStageFlags pipelineStageFlags, VkQueue queue, VulkanSwapchain* pSwapchain = nullptr);

		public:
			/**
			 * Move assignment operator.
			 *
			 * @param other The other command buffer.
			 * @return The moved buffer reference.
			 */
			VulkanCommandBuffer& operator=(VulkanCommandBuffer&& other) noexcept;

			/**
			 * VkCommandBuffer operator.
			 * This can be used to conveniently get the Vulkan command buffer handle.
			 *
			 * @return The Vulkan command buffer.
			 */
			operator VkCommandBuffer() const noexcept { return m_CommandBuffer; }

			/**
			 * VkFence operator.
			 * This can be used to conveniently get the Vulkan fence handle.
			 *
			 * @return The Vulkan fence.
			 */
			operator VkFence() const noexcept { return m_Fence; }

		public:
			/**
			 * Get the command buffer.
			 *
			 * @return The command buffer.
			 */
			XENON_NODISCARD VkCommandBuffer getCommandBuffer() const noexcept { return m_CommandBuffer; }

			/**
			 * Get the command buffer address (pointer).
			 *
			 * @return The const command buffer pointer.
			 */
			XENON_NODISCARD const VkCommandBuffer* getCommandBufferAddress() const noexcept { return &m_CommandBuffer; }

			/**
			 * Get the signal semaphore.
			 *
			 * @return The semaphore.
			 */
			XENON_NODISCARD VkSemaphore getSignalSemaphore() const noexcept { return m_SignalSemaphore; }

			/**
			 * Get the stage flags.
			 *
			 * @return The pipeline stage flags.
			 */
			XENON_NODISCARD VkPipelineStageFlags getStageFlags() const noexcept { return m_StageFlags; }

		private:
			VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
			VkCommandPool m_CommandPool = VK_NULL_HANDLE;

			VkSemaphore m_SignalSemaphore = VK_NULL_HANDLE;

			VkFence m_Fence = VK_NULL_HANDLE;

			VkPipelineStageFlags m_StageFlags = 0;

			bool m_IsFenceFree = true;
		};
	}
}