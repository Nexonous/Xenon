// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../XenonBackend/IFactory.hpp"

namespace Xenon
{
	namespace Backend
	{
		/**
		 * DirectX 12 factory class.
		 * This is used to create DirectX 12 backend objects and is used by the abstraction layer and the frontend.
		 */
		class DX12Factory final : public IFactory
		{
		public:
			/**
			 * Default constructor.
			 */
			DX12Factory() = default;

			/**
			 * Default destructor.
			 */
			~DX12Factory() override = default;

			/**
			 * Create a new instance.
			 *
			 * @param appliationName The name of the application.
			 * @param applicationVersion The application version.
			 * @return The instance pointer.
			 */
			XENON_NODISCARD std::unique_ptr<Instance> createInstance(const std::string& applicationName, uint32_t applicationVersion) override;

			/**
			 * Create a new device.
			 *
			 * @param pInstance The instance pointer to which the device is bound to.
			 * @param requriedRenderTargets The render targets which are required to have.
			 * @return The device pointer.
			 */
			XENON_NODISCARD std::unique_ptr<Device> createDevice(Instance* pInstance, RenderTargetType requiredRenderTargets) override;

			/**
			 * Create a new command recorder.
			 *
			 * @param pDevice The device pointer.
			 * @param usage The command recorder usage.
			 * @param bufferCount The backend primitive buffer count. Default is 1.
			 * @return The command recorder pointer.
			 */
			XENON_NODISCARD std::unique_ptr<CommandRecorder> createCommandRecorder(Device* pDevice, CommandRecorderUsage usage, uint32_t bufferCount = 1) override;

			/**
			 * Create a new index buffer.
			 *
			 * @param pDevice The device pointer.
			 * @param size The size of the buffer in bytes.
			 * @param type The buffer type.
			 * @return The buffer pointer.
			 */
			XENON_NODISCARD std::unique_ptr<Buffer> createBuffer(Device* pDevice, uint64_t size, BufferType type) override;

			/**
			 * Create a new image.
			 *
			 * @param pDevice The device pointer.
			 * @param specification The image specification.
			 * @return The image pointer.
			 */
			XENON_NODISCARD std::unique_ptr<Image> createImage(Device* pDevice, const ImageSpecification& specification) override;

			/**
			 * Create a new rasterizer.
			 *
			 * @param pDevice The device pointer.
			 * @param width The width of the render target.
			 * @param height The height of the render target.
			 * @param attachmentTypes The attachment types the render target should support.
			 * @param enableTripleBuffering Whether to enable triple-buffering. Default is false.
			 * @param multiSampleCount Multi-sampling count to use. Default is x1.
			 * @return The rasterizer pointer.
			 */
			XENON_NODISCARD std::unique_ptr<Rasterizer> createRasterizer(Device* pDevice, uint32_t width, uint32_t height, AttachmentType attachmentTypes, bool enableTripleBuffering = false, MultiSamplingCount multiSampleCount = MultiSamplingCount::x1) override;

			/**
			 * Create a new swapchain.
			 *
			 * @param pDevice The device pointer.
			 * @param title The title of the window.
			 * @param width The window's width.
			 * @param height The window's height.
			 * @return The swapchain pointer.
			 */
			XENON_NODISCARD std::unique_ptr<Swapchain> createSwapchain(Device* pDevice, const std::string& title, uint32_t width, uint32_t height) override;

			/**
			 * Create a new image view.
			 *
			 * @param pDevice The device pointer.
			 * @param pImage The image pointer.
			 * @param specification The view specification.
			 * @return The image view pointer.
			 */
			XENON_NODISCARD std::unique_ptr<ImageView> createImageView(Device* pDevice, Image* pImage, const ImageViewSpecification& specification) override;

			/**
			 * Create a new image sampler.
			 *
			 * @param pDevice The device pointer.
			 * @param specification The sampler specification.
			 * @return The image sampler pointer.
			 */
			XENON_NODISCARD std::unique_ptr<ImageSampler> createImageSampler(Device* pDevice, const ImageSamplerSpecification& specification) override;

			/**
			 * Create a new rasterizing pipeline.
			 *
			 * @param pDevice The device pointer.
			 * @param pCacheHandler The cache handler pointer.
			 * @param pRasterizer The rasterizer pointer.
			 * @param specification The pipeline specification.
			 * @return The pipeline pointer.
			 */
			XENON_NODISCARD std::unique_ptr<RasterizingPipeline> createRasterizingPipeline(Device* pDevice, std::unique_ptr<PipelineCacheHandler>&& pCacheHandler, Rasterizer* pRasterizer, const RasterizingPipelineSpecification& specification) override;

			/**
			 * Create a new compute pipeline.
			 *
			 * @param pDevice The device pointer.
			 * @param pCacheHandler The cache handler pointer. This can be null in which case the pipeline creation might get slow.
			 * @param computeShader The compute shader.
			 * @return The pipeline pointer.
			 */
			XENON_NODISCARD std::unique_ptr<ComputePipeline> createComputePipeline(Device* pDevice, std::unique_ptr<PipelineCacheHandler>&& pCacheHandler, const Shader& computeShader) override;

			/**
			 * Create a new command submitter.
			 *
			 * @param pDevice The device pointer.
			 * @return The command submitter pointer.
			 */
			XENON_NODISCARD std::unique_ptr<CommandSubmitter> createCommandSubmitter(Device* pDevice) override;

			/**
			 * Create a new occlusion query.
			 *
			 * @param pDevice The device pointer.
			 * @param sampleCount The maximum sample count the query can hold.
			 * @return The occlusion query pointer.
			 */
			XENON_NODISCARD std::unique_ptr<OcclusionQuery> createOcclusionQuery(Device* pDevice, uint64_t sampleCount) override;

			/**
			 * Create a new top level acceleration structure.
			 *
			 * @param pDevice The device pointer.
			 * @param pBottomLevelAccelerationStructures The bottom level acceleration structures.
			 * @return The acceleration structure pointer.
			 */
			XENON_NODISCARD std::unique_ptr<TopLevelAccelerationStructure> createTopLevelAccelerationStructure(Device* pDevice, const std::vector<BottomLevelAccelerationStructure*>& pBottomLevelAccelerationStructures) override;

			/**
			 * Create a new bottom level acceleration structure.
			 *
			 * @param pDevice The device pointer.
			 * @param geometries The geometries to store.
			 * @return The acceleration structure pointer.
			 */
			XENON_NODISCARD std::unique_ptr<BottomLevelAccelerationStructure> createBottomLevelAccelerationStructure(Device* pDevice, const std::vector<AccelerationStructureGeometry>& geometries) override;

			/**
			 * Create a new ray tracer.
			 *
			 * @param pDevice The device pointer.
			 * @param width The width of the render target.
			 * @param height The height of the render target.
			 * @return The ray tracer pointer.
			 */
			XENON_NODISCARD std::unique_ptr<RayTracer> createRayTracer(Device* pDevice, uint32_t width, uint32_t height) override;

			/**
			 * Create anew ray tracing pipeline.
			 *
			 * @param pDevice The device pointer.
			 * @param pCacheHandler The cache handler pointer. This can be null in which case the pipeline creation might get slow.
			 * @param specification The pipeline specification.
			 * @return The pipeline pointer.
			 */
			XENON_NODISCARD std::unique_ptr<RayTracingPipeline> createRayTracingPipeline(Device* pDevice, std::unique_ptr<PipelineCacheHandler>&& pCacheHandler, const RayTracingPipelineSpecification& specification) override;
		};
	}
}