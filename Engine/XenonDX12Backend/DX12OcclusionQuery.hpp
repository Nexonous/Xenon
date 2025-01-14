// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../XenonBackend/OcclusionQuery.hpp"

#include "DX12DeviceBoundObject.hpp"

namespace Xenon
{
	namespace Backend
	{
		/**
		 * DirectX 12 occlusion query class.
		 */
		class DX12OcclusionQuery final : public OcclusionQuery, public DX12DeviceBoundObject
		{
		public:
			/**
			 * Explicit constructor.
			 *
			 * @param pDevice The device pointer.
			 * @param sampleCount The number of possible samples.
			 */
			explicit DX12OcclusionQuery(DX12Device* pDevice, uint64_t sampleCount);

			/**
			 * Destructor.
			 */
			~DX12OcclusionQuery() override;

			/**
			 * Get the samples.
			 * This will query the samples from the backend.
			 *
			 * @return The samples.
			 */
			XENON_NODISCARD std::vector<uint64_t> getSamples() override;

			/**
			 * Get the occlusion query heap.
			 *
			 * @return The heap pointer.
			 */
			XENON_NODISCARD ID3D12QueryHeap* getHeap() noexcept { return m_QueryHeap.Get(); }

			/**
			 * Get the occlusion query heap.
			 *
			 * @return The heap pointer.
			 */
			XENON_NODISCARD const ID3D12QueryHeap* getHeap() const noexcept { return m_QueryHeap.Get(); }

			/**
			 * Get the resource buffer pointer.
			 *
			 * @return The resource pointer.
			 */
			XENON_NODISCARD ID3D12Resource* getBuffer() noexcept { return m_pAllocation->GetResource(); }

			/**
			 * Get the resource buffer pointer.
			 *
			 * @return The resource pointer.
			 */
			XENON_NODISCARD const ID3D12Resource* getBuffer() const noexcept { return m_pAllocation->GetResource(); }

		private:
			ComPtr<ID3D12QueryHeap> m_QueryHeap;

			D3D12MA::Allocation* m_pAllocation = nullptr;
		};
	}
}