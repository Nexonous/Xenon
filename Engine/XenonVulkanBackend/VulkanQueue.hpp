// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../XenonCore/Common.hpp"

#include <volk.h>

namespace Xenon
{
	namespace Backend
	{
		/**
		 * Vulkan queue class.
		 */
		class VulkanQueue final
		{
		public:
			/**
			 * Default constructor.
			 */
			VulkanQueue() = default;

			/**
			 * Default destructor.
			 */
			~VulkanQueue() = default;

			/**
			 * Find the best queue family.
			 *
			 * @param physicalDevice The physical device to which the queue is bound to.
			 * @param flag The queue flag.
			 * @return The queue family.
			 */
			XENON_NODISCARD static uint32_t FindFamily(VkPhysicalDevice physicalDevice, VkQueueFlagBits flag);

			/**
			 * Set the queue's family.
			 *
			 * @param family The queue family.
			 */
			void setFamily(uint32_t family);

			/**
			 * Set the queue.
			 *
			 * @param queue The queue to set.
			 */
			void setQueue(VkQueue queue);

		public:
			/**
			 * Get the queue family.
			 *
			 * @return The optional family.
			 */
			XENON_NODISCARD uint32_t getFamily() const { return m_Family; }

			/**
			 * Get the internally stored queue.
			 *
			 * @return The queue.
			 */
			XENON_NODISCARD VkQueue getQueue() const { return m_Queue; }

		private:
			VkQueue m_Queue = VK_NULL_HANDLE;
			uint32_t m_Family = -1;
		};
	}
}