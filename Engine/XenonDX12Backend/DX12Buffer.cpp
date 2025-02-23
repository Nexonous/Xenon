// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "DX12Buffer.hpp"
#include "DX12Macros.hpp"
#include "DX12CommandRecorder.hpp"

#include <optick.h>

namespace Xenon
{
	namespace Backend
	{
		DX12Buffer::DX12Buffer(DX12Device* pDevice, uint64_t size, BufferType type)
			: Buffer(pDevice, size, type)
			, DX12DeviceBoundObject(pDevice)
		{
			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			CD3DX12_RESOURCE_DESC resourceDescriptor = {};

			switch (type)
			{
			case Xenon::Backend::BufferType::Index:
				m_CurrentState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
				allocationDesc.HeapType = m_HeapType;
				resourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(size);
				break;

			case Xenon::Backend::BufferType::Vertex:
				m_CurrentState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
				allocationDesc.HeapType = m_HeapType;
				resourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(size);
				break;

			case Xenon::Backend::BufferType::Staging:
				m_CurrentState = D3D12_RESOURCE_STATE_COMMON;
				allocationDesc.HeapType = m_HeapType;
				resourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(size);
				break;

			case Xenon::Backend::BufferType::Storage:
				m_CurrentState = D3D12_RESOURCE_STATE_COMMON;
				allocationDesc.HeapType = m_HeapType;
				resourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(size);
				break;

			case Xenon::Backend::BufferType::Uniform:
				m_CurrentState = D3D12_RESOURCE_STATE_COMMON;
				allocationDesc.HeapType = m_HeapType;
				m_Size = static_cast<uint64_t>(std::ceil(static_cast<float>(size) / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) * D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
				resourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(m_Size);
				break;

			default:
				m_Type = BufferType::Staging;
				m_CurrentState = D3D12_RESOURCE_STATE_COMMON;
				allocationDesc.HeapType = m_HeapType;
				resourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(size);
				XENON_LOG_ERROR("Invalid or unsupported buffer type! Defaulting to staging.");
				break;
			}

			XENON_DX12_ASSERT(pDevice->getAllocator()->CreateResource(
				&allocationDesc,
				&resourceDescriptor,
				m_CurrentState,
				nullptr,
				&m_pAllocation,
				IID_NULL,
				nullptr), "Failed to create the buffer!");

			XENON_DX12_NAME_OBJECT(getResource(), "Buffer");

			// Setup the command structures.
			setupCommandStructures();
		}

		DX12Buffer::DX12Buffer(DX12Device* pDevice, uint64_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES resourceStates, D3D12_RESOURCE_FLAGS resourceFlags /*= D3D12_RESOURCE_FLAG_NONE*/)
			: Buffer(pDevice, size, BufferType::BackendSpecific)
			, DX12DeviceBoundObject(pDevice)
			, m_CurrentState(resourceStates)
			, m_HeapType(heapType)
		{
			CD3DX12_RESOURCE_DESC resourceDescriptor = CD3DX12_RESOURCE_DESC::Buffer(size, resourceFlags);

			D3D12MA::ALLOCATION_DESC allocationDesc = {};
			allocationDesc.HeapType = heapType;

			XENON_DX12_ASSERT(pDevice->getAllocator()->CreateResource(
				&allocationDesc,
				&resourceDescriptor,
				m_CurrentState,
				nullptr,
				&m_pAllocation,
				IID_NULL,
				nullptr), "Failed to create the buffer!");

			XENON_DX12_NAME_OBJECT(getResource(), "Buffer");

			// Setup the command structures.
			setupCommandStructures();
		}

		DX12Buffer::~DX12Buffer()
		{
			m_pAllocation->Release();
		}

		void DX12Buffer::copy(Buffer* pBuffer, uint64_t size, uint64_t srcOffset /*= 0*/, uint64_t dstOffset /*= 0*/)
		{
			OPTICK_EVENT();

			// Begin the command list.
			XENON_DX12_ASSERT(m_CommandAllocator->Reset(), "Failed to reset the current command allocator!");
			XENON_DX12_ASSERT(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr), "Failed to reset the current command list!");

			// Perform the copy.
			performCopy(m_CommandList.Get(), pBuffer, size, srcOffset, dstOffset);

			// End the command list.
			XENON_DX12_ASSERT(m_CommandList->Close(), "Failed to stop the current command list!");

			// Submit the command list to be executed.
			std::array<ID3D12CommandList*, 1> ppCommandLists = { m_CommandList.Get() };
			m_pDevice->getDirectQueue()->ExecuteCommandLists(ppCommandLists.size(), ppCommandLists.data());

			// Wait till the command is done executing.
			ComPtr<ID3D12Fence> fence;
			XENON_DX12_ASSERT(m_pDevice->getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "Failed to create the fence!");
			XENON_DX12_ASSERT(m_pDevice->getDirectQueue()->Signal(fence.Get(), 1), "Failed to signal the fence!");

			// Setup synchronization.
			auto fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			// Validate the event.
			if (fenceEvent == nullptr)
			{
				XENON_LOG_ERROR("Failed to wait till the command list execution!");
				return;
			}

			// Set the event and wait.
			XENON_DX12_ASSERT(fence->SetEventOnCompletion(1, fenceEvent), "Failed to set the fence event on completion event!");
			WaitForSingleObjectEx(fenceEvent, std::numeric_limits<DWORD>::max(), FALSE);
			CloseHandle(fenceEvent);
		}

		void DX12Buffer::write(const std::byte* pData, uint64_t size, uint64_t offset /*= 0*/, CommandRecorder* pCommandRecorder /*= nullptr*/)
		{
			OPTICK_EVENT();

			// If we are made to copy, just copy.
			if (m_HeapType == D3D12_HEAP_TYPE_UPLOAD && m_CurrentState == D3D12_RESOURCE_STATE_GENERIC_READ)
			{
				// Map the memory.
				std::byte* copyBufferMemory = nullptr;
				getResource()->Map(0, nullptr, XENON_BIT_CAST(void**, &copyBufferMemory));

				// Copy the data to the copy buffer.
				std::copy_n(pData, size, copyBufferMemory + offset);

				// Unmap the copy buffer.
				getResource()->Unmap(0, nullptr);
			}

			// Else copy with the temp buffer.
			else
			{
				// Create the temporary buffer if it's null.
				if (m_pTemporaryWriteBuffer == nullptr)
					m_pTemporaryWriteBuffer = std::make_unique<DX12Buffer>(m_pDevice, m_Size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

				// Map the memory.
				std::byte* copyBufferMemory = nullptr;
				m_pTemporaryWriteBuffer->getResource()->Map(0, nullptr, XENON_BIT_CAST(void**, &copyBufferMemory));

				// Copy the data to the copy buffer.
				std::copy_n(pData, size, copyBufferMemory + offset);

				// Unmap the copy buffer.
				m_pTemporaryWriteBuffer->getResource()->Unmap(0, nullptr);

				// Finally copy everything to this.
				if (pCommandRecorder)
					performCopy(pCommandRecorder->as<DX12CommandRecorder>()->getCurrentCommandList(), m_pTemporaryWriteBuffer.get(), size, offset, offset);

				else
					copy(m_pTemporaryWriteBuffer.get(), size, offset, offset);
			}
		}

		const std::byte* DX12Buffer::beginRead()
		{
			OPTICK_EVENT();

			return map();
		}

		void DX12Buffer::endRead()
		{
			OPTICK_EVENT();

			unmap();
		}

		const std::byte* DX12Buffer::map()
		{
			OPTICK_EVENT();

			// Create the temporary buffer if it's null.
			if (m_pTemporaryReadBuffer == nullptr)
				m_pTemporaryReadBuffer = std::make_unique<DX12Buffer>(m_pDevice, m_Size, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);

			// Copy the current data to it.
			m_pTemporaryReadBuffer->copy(this, m_Size, 0, 0);

			// Map the resources now.
			std::byte* tempBufferMemory = nullptr;
			m_pTemporaryReadBuffer->getResource()->Map(0, nullptr, XENON_BIT_CAST(void**, &tempBufferMemory));
			return tempBufferMemory;
		}

		void DX12Buffer::unmap()
		{
			OPTICK_EVENT();

			m_pTemporaryReadBuffer->getResource()->Unmap(0, nullptr);
		}

		void DX12Buffer::performCopy(ID3D12GraphicsCommandList* pCommandlist, Buffer* pBuffer, uint64_t size, uint64_t srcOffset /*= 0*/, uint64_t dstOffset /*= 0*/)
		{
			OPTICK_EVENT();

			auto pSourceBuffer = pBuffer->as<DX12Buffer>();

			// Set the proper resource states.
			// Destination (this)
			if (m_CurrentState != D3D12_RESOURCE_STATE_GENERIC_READ && m_HeapType != D3D12_HEAP_TYPE_READBACK)
			{
				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pAllocation->GetResource(), m_CurrentState, D3D12_RESOURCE_STATE_COPY_DEST);
				pCommandlist->ResourceBarrier(1, &barrier);
			}

			// Source
			if (pSourceBuffer->m_CurrentState != D3D12_RESOURCE_STATE_GENERIC_READ && pSourceBuffer->m_HeapType != D3D12_HEAP_TYPE_READBACK)
			{
				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSourceBuffer->getResource(), pSourceBuffer->m_CurrentState, D3D12_RESOURCE_STATE_COPY_SOURCE);
				pCommandlist->ResourceBarrier(1, &barrier);
			}

			// Copy the buffer region.
			pCommandlist->CopyBufferRegion(m_pAllocation->GetResource(), dstOffset, pSourceBuffer->getResource(), srcOffset, size);

			// Change the state back to previous.
			// Destination (this)
			if (m_CurrentState != D3D12_RESOURCE_STATE_GENERIC_READ && m_HeapType != D3D12_HEAP_TYPE_READBACK)
			{
				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pAllocation->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, m_CurrentState);
				pCommandlist->ResourceBarrier(1, &barrier);
			}

			// Source
			if (pSourceBuffer->m_CurrentState != D3D12_RESOURCE_STATE_GENERIC_READ && pSourceBuffer->m_HeapType != D3D12_HEAP_TYPE_READBACK)
			{
				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSourceBuffer->getResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, pSourceBuffer->m_CurrentState);
				pCommandlist->ResourceBarrier(1, &barrier);
			}
		}

		void DX12Buffer::setupCommandStructures()
		{
			// Create the allocator.
			XENON_DX12_ASSERT(m_pDevice->getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)), "Failed to create the copy command allocator!");
			XENON_DX12_NAME_OBJECT(m_CommandAllocator, "Buffer Command Allocator");

			// Create the command list.
			XENON_DX12_ASSERT(m_pDevice->getDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)), "Failed to create the copy command list!");
			XENON_DX12_NAME_OBJECT(m_CommandList, "Buffer Command List");

			// End the command list.
			XENON_DX12_ASSERT(m_CommandList->Close(), "Failed to stop the current command list!");
		}
	}
}