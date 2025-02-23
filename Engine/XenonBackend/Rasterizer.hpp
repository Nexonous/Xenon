// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "RenderTarget.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <variant>

namespace Xenon
{
	namespace Backend
	{
		/**
		 * Rasterizer class.
		 * This class can be used to perform rasterization on a scene.
		 */
		class Rasterizer : public RenderTarget
		{
		public:
			using ClearValueType = std::variant<
				glm::vec4,								// Color attachment clear value.
				glm::vec3,								// Normal map clear value.
				float,									// Depth or Entity ID clear value.
				uint32_t								// Stencil clear value.
			>;

		public:
			/**
			 * Explicit constructor.
			 *
			 * @param pDevice The device pointer.
			 * @param width The width of the render target.
			 * @param height The height of the render target.
			 * @param attachmentTypes The attachment types the render target should support.
			 * @param enableTripleBuffering Whether to enable triple-buffering. Default is false.
			 * @param multiSampleCount Multi-sampling count to use. Default is x1.
			 */
			explicit Rasterizer(const Device* pDevice, uint32_t width, uint32_t height, AttachmentType attachmentTypes, bool enableTripleBuffering = false, MultiSamplingCount multiSampleCount = MultiSamplingCount::x1)
				: RenderTarget(pDevice, width, height, attachmentTypes), m_bEnableTripleBuffering(enableTripleBuffering), m_MultiSamplingCount(multiSampleCount) {}

		public:
			/**
			 * Check if triple buffering is enabled.
			 *
			 * @return True if triple buffering is enabled.
			 * @return False if triple buffering is not enabled.
			 */
			XENON_NODISCARD bool isTripleBufferingEnabled() const { return m_bEnableTripleBuffering; }

			/**
			 * Get the enabled multi-sampling count.
			 *
			 * @return The multi-sampling count.
			 */
			XENON_NODISCARD MultiSamplingCount getMultiSamplingCount() const { return m_MultiSamplingCount; }

			/**
			 * Get the current frame index.
			 *
			 * @return The frame index.
			 */
			XENON_NODISCARD uint32_t getFrameIndex() const { return m_FrameIndex; }

		protected:
			uint32_t m_FrameIndex = 0;

			bool m_bEnableTripleBuffering = false;
			MultiSamplingCount m_MultiSamplingCount = MultiSamplingCount::x1;
		};
	}
}