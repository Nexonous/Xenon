// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../RasterizingLayer.hpp"

namespace Xenon
{
	namespace Experimental
	{
		/**
		 * GBuffer face enum.
		 */
		enum class GBufferFace : uint8_t
		{
			PositiveX,	// +X
			NegativeX,	// -X

			PositiveY,	// +Y
			NegativeY,	// -Y

			PositiveZ,	// +Z
			NegativeZ,	// -Z

			Front = NegativeZ
		};

		/**
		 * GBuffer layer class.
		 * This class can be used to store geometry information, specifically the following
		 * 1. Color image.
		 * 2. Depth image.
		 * 3. Normal image.
		 * 4. Roughness image.
		 *
		 * This additionally takes a face since the geometry information can be computed to 6 faces (360 degrees).
		 */
		class GBufferLayer final : public RasterizingLayer
		{
		public:
			/**
			 * Explicit constructor.
			 *
			 * @param renderer The renderer reference.
			 * @param width The width of the render target.
			 * @param height The height of the render target.
			 * @param face The camera's current face. Default is front (no change to the camera).
			 * @param priority The priority of the layer. Default is 0.
			 */
			explicit GBufferLayer(Renderer& renderer, uint32_t width, uint32_t height, GBufferFace face = GBufferFace::Front, uint32_t priority = 0);

			/**
			 * On pre-update function.
			 * This object is called by the renderer before issuing it to the job system to be executed.
			 */
			void onPreUpdate() override;

			/**
			 * Update the layer.
			 * This is called by the renderer and all the required commands must be updated (if required) in this call.
			 *
			 * @param pPreviousLayer The previous layer pointer. This will be nullptr if this layer is the first.
			 * @param imageIndex The image's index.
			 * @param frameIndex The frame's index.
			 */
			void onUpdate(Layer* pPreviousLayer, uint32_t imageIndex, uint32_t frameIndex) override;

			/**
			 * Set the renderable scene to the layer.
			 *
			 * @param scene The scene reference.
			 */
			void setScene(Scene& scene) override;

			/**
			 * Get the normal attachment.
			 *
			 * @return The normal attachment
			 */
			XENON_NODISCARD Backend::Image* getNormalAttachment() { return m_pRasterizer->getImageAttachment(Backend::AttachmentType::Normal); }

			/**
			 * Get the position attachment.
			 *
			 * @return The position attachment
			 */
			XENON_NODISCARD Backend::Image* getPositionAttachment() { return m_pRasterizer->getImageAttachment(Backend::AttachmentType::Position); }

			/**
			 * Get the GBuffer face.
			 *
			 * @return The face.
			 */
			XENON_NODISCARD GBufferFace getFace() const noexcept { return m_Face; }

		private:
			/**
			 * Issue the required draw calls.
			 */
			void issueDrawCalls();

			/**
			 * Bind everything and perform the draw.
			 *
			 * @param subMesh The sub-mesh to draw.
			 * @param geometry The geometry to draw.
			 */
			void performDraw(const SubMesh& subMesh, Geometry& geometry);

			/**
			 * Create a new material descriptor.
			 *
			 * @param subMesh The sub-mesh of the material.
			 */
			void createMaterial(SubMesh& subMesh);

			/**
			 * Rotate the camera to the required face.
			 */
			void rotateCamera();

		private:
			glm::mat4 m_RotationMatrix = glm::mat4(1.0f);

			std::unique_ptr<Backend::Image> m_pLightImage = nullptr;

			std::unique_ptr<Backend::Buffer> m_pRotationBuffer = nullptr;

			std::unique_ptr<Backend::RasterizingPipeline> m_pPipeline = nullptr;

			std::unique_ptr<Backend::Descriptor> m_pUserDefinedDescriptor = nullptr;
			std::unique_ptr<Backend::Descriptor> m_pSceneDescriptor = nullptr;
			std::unordered_map<SubMesh, std::unique_ptr<Backend::Descriptor>> m_pMaterialDescriptors;

			GBufferFace m_Face = GBufferFace::Front;
		};
	}
}