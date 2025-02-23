// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../XenonBackend/OcclusionQuery.hpp"

#include "VulkanDeviceBoundObject.hpp"

namespace Xenon
{
	namespace Backend
	{
		/**
		 * Vulkan occlusion query class.
		 */
		class VulkanOcclusionQuery final : public OcclusionQuery, public VulkanDeviceBoundObject
		{
		public:
			/**
			 * Explicit constructor.
			 *
			 * @param pDevice The device pointer.
			 * @param sampleCount The number of possible samples.
			 */
			explicit VulkanOcclusionQuery(VulkanDevice* pDevice, uint64_t sampleCount);

			/**
			 * Destructor.
			 */
			~VulkanOcclusionQuery() override;

			/**
			 * Get the samples.
			 * This will query the samples from the backend.
			 *
			 * @return The samples.
			 */
			XENON_NODISCARD std::vector<uint64_t> getSamples() override;

			/**
			 * Get the query pool.
			 *
			 * @return The query pool.
			 */
			XENON_NODISCARD VkQueryPool getQueryPool() const noexcept { return m_QueryPool; }

		private:
			VkQueryPool m_QueryPool = VK_NULL_HANDLE;
		};
	}
}