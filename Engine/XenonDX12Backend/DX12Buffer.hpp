// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../XenonBackend/Buffer.hpp"

#include "DX12DeviceBoundObject.hpp"

namespace Xenon
{
	namespace Backend
	{
		class DX12CommandRecorder;

		/**
		 * DirectX 12 buffer class.
		 * This is the base class for all the DirectX 12 buffers.
		 */
		class DX12Buffer final : public Buffer, public DX12DeviceBoundObject
		{
		public:
			/**
			 * Explicit constructor.
			 *
			 * @param pDevice The device pointer.
			 * @param size The size of the buffer in bytes.
			 * @param type The buffer type.
			 */
			explicit DX12Buffer(DX12Device* pDevice, uint64_t size, BufferType type);

			/**
			 * Explicit constructor.
			 * Note that this is an internal constructor and is not exposed across the backend layer.
			 *
			 * @param pDevice The device pointer.
			 * @param size The size of the buffer.
			 * @param heapType The type of the heap.
			 * @param resourceStates The resource states.
			 * @param resourceFlags The buffer resource flags. Default is none.
			 */
			explicit DX12Buffer(DX12Device* pDevice, uint64_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES resourceStates, D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE);

			/**
			 * Default destructor.
			 */
			~DX12Buffer() override;

			/**
			 * Copy data from another buffer to this buffer.
			 *
			 * @param pBuffer The buffer to copy the data from.
			 * @param size The size in bytes to copy.
			 * @param srcOffset The source buffer's offset. Default is 0.
			 * @param dstOffset The destination buffer's (this) offset. Default is 0.
			 */
			void copy(Buffer* pBuffer, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0) override;

			/**
			 * Write data to the buffer.
			 *
			 * @param pData The data pointer to copy the data from.
			 * @param size The size of the data to copy in bytes.
			 * @param offset The buffer's offset to copy to. Default is 0.
			 * @param pCommandRecorder The command recorder used for internal transfer. Default is nullptr.
			 */
			void write(const std::byte* pData, uint64_t size, uint64_t offset = 0, CommandRecorder* pCommandRecorder = nullptr) override;

			/**
			 * Begin reading data from the GPU.
			 *
			 * @return The const data pointer.
			 */
			XENON_NODISCARD const std::byte* beginRead() override;

			/**
			 * End the buffer reading.
			 */
			void endRead() override;

		private:
			/**
			 * Map the buffer memory to the local address space.
			 *
			 * @return The buffer memory.
			 */
			const std::byte* map();

			/**
			 * Unmap the buffer memory.
			 */
			void unmap();

			/**
			 * Copy the resources using a command recorder.
			 *
			 * @param pCommandlist The command list used for transfer.
			 * @param pBuffer The buffer to copy the data from.
			 * @param size The size in bytes to copy.
			 * @param srcOffset The source buffer's offset. Default is 0.
			 * @param dstOffset The destination buffer's (this) offset. Default is 0.
			 */
			void performCopy(ID3D12GraphicsCommandList* pCommandlist, Buffer* pBuffer, uint64_t size, uint64_t srcOffset = 0, uint64_t dstOffset = 0);

		public:
			/**
			 * Get the internally managed resource.
			 *
			 * @return The resource pointer.
			 */
			XENON_NODISCARD ID3D12Resource* getResource() { return m_pAllocation->GetResource(); }

			/**
			 * Get the internally managed resource.
			 *
			 * @return The const resource pointer.
			 */
			XENON_NODISCARD const ID3D12Resource* getResource() const { return m_pAllocation->GetResource(); }

			/**
			 * Get the current resource state.
			 *
			 * @return The resource state.
			 */
			XENON_NODISCARD D3D12_RESOURCE_STATES getResourceState() const noexcept { return m_CurrentState; }

		private:
			/**
			 * Setup the required command structures.
			 */
			void setupCommandStructures();

		private:
			D3D12MA::Allocation* m_pAllocation = nullptr;

			ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
			ComPtr<ID3D12GraphicsCommandList> m_CommandList;

			std::unique_ptr<DX12Buffer> m_pTemporaryReadBuffer = nullptr;
			std::unique_ptr<DX12Buffer> m_pTemporaryWriteBuffer = nullptr;

			D3D12_RESOURCE_STATES m_CurrentState = D3D12_RESOURCE_STATE_COMMON;
			D3D12_HEAP_TYPE m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
		};
	}
}