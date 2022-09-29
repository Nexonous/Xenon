// Copyright 2022 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VulkanDeviceBoundObject.hpp"

namespace Xenon
{
	namespace Backend
	{
		/**
		 * Vulkan buffer class.
		 * This class is a universal buffer which can be created to be any type.
		 */
		class VulkanBuffer : public VulkanDeviceBoundObject
		{
		public:
			/**
			 * Explicit constructor.
			 *
			 * @param pDevice The device pointer.
			 * @param size The size of the buffer.
			 * @param usageFlags The buffer usage flags.
			 * @param memoryUsage The memory usage flags.
			 */
			explicit VulkanBuffer(VulkanDevice* pDevice, uint64_t size, VkBufferUsageFlags usageFlags, VmaMemoryUsage memoryUsage);

			/**
			 * Destructor.
			 */
			~VulkanBuffer() override;

		public:
			/**
			 * Get the Vulkan buffer.
			 *
			 * @return The Vulkan buffer.
			 */
			[[nodiscard]] VkBuffer getBuffer() const { return m_Buffer; }

			/**
			 * Get the VMA allocation.
			 *
			 * @return The allocation.
			 */
			[[nodiscard]] VmaAllocation getAllocation() const { return m_Allocation; }

			/**
			 * Get the descriptor buffer info.
			 *
			 * @return The buffer info.
			 */
			[[nodiscard]] VkDescriptorBufferInfo getDescriptorBufferInfo() const { return m_BufferInfo; }

		protected:
			VkDescriptorBufferInfo m_BufferInfo;

			VkBuffer m_Buffer = VK_NULL_HANDLE;
			VmaAllocation m_Allocation = nullptr;
		};
	}
}