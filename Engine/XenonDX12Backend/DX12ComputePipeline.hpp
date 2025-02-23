// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../XenonBackend/ComputePipeline.hpp"

#include "DX12DescriptorHeapManager.hpp"

namespace Xenon
{
	namespace Backend
	{
		/**
		 * DirectX 12 compute pipeline class.
		 */
		class DX12ComputePipeline final : public ComputePipeline, public DX12DescriptorHeapManager
		{
		public:
			/**
			 * Explicit constructor.
			 *
			 * @param pDevice The device pointer.
			 * @param pCacheHandler The cache handler pointer. This can be null in which case the pipeline creation might get slow.
			 * @param computeShader The compute shader.
			 */
			explicit DX12ComputePipeline(DX12Device* pDevice, std::unique_ptr<PipelineCacheHandler>&& pCacheHandler, const Shader& computeShader);

			/**
			 * Destructor.
			 */
			~DX12ComputePipeline() override = default;

			/**
			 * Create a new descriptor.
			 *
			 * @param type The descriptor type to create.
			 * @return The descriptor pointer. It will return nullptr if the descriptor type is not present in the pipeline.
			 */
			XENON_NODISCARD std::unique_ptr<Descriptor> createDescriptor(DescriptorType type) override;

			/**
			 * Get the root signature object.
			 *
			 * @return The root signature pointer.
			 */
			XENON_NODISCARD ID3D12RootSignature* getRootSignature() noexcept { return m_RootSignature.Get(); }

			/**
			 * Get the root signature object.
			 *
			 * @return The root signature pointer.
			 */
			XENON_NODISCARD const ID3D12RootSignature* getRootSignature() const noexcept { return m_RootSignature.Get(); }

			/**
			 * Get the pipeline state.
			 *
			 * @return The pipeline state pointer.
			 */
			XENON_NODISCARD ID3D12PipelineState* getPipelineState() noexcept { return m_PipelineState.Get(); }

			/**
			 * Get the pipeline state.
			 *
			 * @return The pipeline state pointer.
			 */
			XENON_NODISCARD const ID3D12PipelineState* getPipelineState() const noexcept { return m_PipelineState.Get(); }

		private:
			/**
			 * Create the root signature.
			 *
			 * @param descriptorRanges The descriptor ranges.
			 */
			void createRootSignature(std::vector<CD3DX12_DESCRIPTOR_RANGE1>&& descriptorRanges);

			/**
			 * Load the pipeline state cache.
			 *
			 * @return The pipeline state cache.
			 */
			XENON_NODISCARD std::vector<std::byte> loadPipelineSateCache() const;

			/**
			 * Store the pipeline state cache using the cache handler.
			 */
			void storePipelineStateCache() const;

			/**
			 * Create the pipeline state object.
			 */
			void createPipelineStateObject();

		private:
			std::unordered_map<uint32_t, DescriptorBindingInfo> m_BindingInfos;
			std::unordered_map<uint32_t, UINT> m_BindingOffsets;

			ComPtr<ID3D12RootSignature> m_RootSignature;
			ComPtr<ID3D12PipelineState> m_PipelineState;

			uint64_t m_PipelineHash = 0;
		};
	}
}