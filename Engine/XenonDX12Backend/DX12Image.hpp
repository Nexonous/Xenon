// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../XenonBackend/Image.hpp"

#include "DX12DeviceBoundObject.hpp"

namespace Xenon
{
	namespace Backend
	{
		/**
		 * Direct X 12 image class.
		 */
		class DX12Image final : public Image, public DX12DeviceBoundObject
		{
		public:
			/**
			 * Explicit constructor.
			 *
			 * @param pDevice The device pointer.
			 * @param specification The image specification.
			 * @param resourceStates The image's resource states. Default is copy destination.
			 * @param heapType The memory heap type. Default is default.
			 * @param heapFlags The heap flags. Default is none.
			 * @param pClearValue The clear value pointer. This is used when the image is used as a render target (color or depth). Default is nullptr.
			 */
			explicit DX12Image(
				DX12Device* pDevice,
				const ImageSpecification& specification,
				D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT,
				D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE,
				D3D12_CLEAR_VALUE* pClearValue = nullptr);

			/**
			 * Move constructor.
			 *
			 * @param other The other image.
			 */
			DX12Image(DX12Image&& other) noexcept;

			/**
			 * Default destructor.
			 */
			~DX12Image() override;

			/**
			 * Copy image data from a source buffer.
			 *
			 * @param pSrcBuffer The source buffer pointer.
			 * @param pCommandRecorder The command recorder pointer to record the commands to. Default is nullptr.
			 */
			void copyFrom(Buffer* pSrcBuffer, CommandRecorder* pCommandRecorder = nullptr) override;

			/**
			 * Copy image data from a source image.
			 *
			 * @param pSrcImage The source image.
			 * @param pCommandRecorder The command recorder pointer to record the commands to. Default is nullptr.
			 */
			void copyFrom(Image* pSrcImage, CommandRecorder* pCommandRecorder = nullptr) override;

			/**
			 * Generate mip maps for the currently stored image.
			 *
			 * @param pCommandRecorder The command recorder pointer to record the commands to. Default is nullptr (in which case the backend will create one for this purpose).
			 */
			void generateMipMaps(CommandRecorder* pCommandRecorder = nullptr) override;

			/**
			 * Get the backend resource.
			 *
			 * @return The resource pointer.
			 */
			XENON_NODISCARD ID3D12Resource* getResource() { return m_pAllocation->GetResource(); }

			/**
			 * Get the backend resource.
			 *
			 * @return The const resource pointer.
			 */
			XENON_NODISCARD const ID3D12Resource* getResource() const { return m_pAllocation->GetResource(); }

			/**
			 * Get the current resource state.
			 *
			 * @return The current resource state.
			 */
			XENON_NODISCARD D3D12_RESOURCE_STATES getCurrentState() const noexcept { return m_CurrentState; }

			/**
			 * Get the sample description of the image.
			 *
			 * @return The sample description used in the image.
			 */
			XENON_NODISCARD DXGI_SAMPLE_DESC getSampleDesc() const noexcept { return m_SampleDesc; }

			/**
			 * Set the current image state.
			 * This must be set after updating the resource state externally or internally.
			 *
			 * @param state The state to set.
			 */
			void setCurrentState(D3D12_RESOURCE_STATES state) { m_CurrentState = state; }

		public:
			/**
			 * Move assign operator.
			 *
			 * @param other The other image.
			 * @return The assigned image reference.
			 */
			DX12Image& operator=(DX12Image&& other) noexcept;

		private:
			D3D12MA::Allocation* m_pAllocation = nullptr;

			ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
			ComPtr<ID3D12GraphicsCommandList> m_CommandList;

			DXGI_SAMPLE_DESC m_SampleDesc = {};

			D3D12_RESOURCE_STATES m_CurrentState;
		};
	}
}