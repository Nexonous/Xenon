// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "LODTree.hpp"

#include "../XenonCore/Logging.hpp"

#include <array>

namespace /* anonymous */
{
	uint32_t GetOptimizedIndexCount(const uint16_t* pBegin, uint32_t indexCount)
	{
		return 1;
	}
}

namespace Xenon
{
	LODTree::LODTree(uint16_t* pBegin, uint16_t* pEnd)
	{
		struct LODNode final
		{
			uint64_t m_Offset = 0;
			uint32_t m_IndexCount = 0;
			uint32_t m_ID = 0;
		};

		const auto levelCount = static_cast<uint8_t>(std::floor(std::log(std::distance(pBegin, pEnd))));
		m_LODOffsets.reserve(levelCount);

		std::vector<LODNode> LOD0;
		for (auto itr = pBegin; itr != pEnd; itr += 3)
		{
			LOD0.emplace_back(std::distance(pBegin, itr), 3, static_cast<uint32_t>(itr - pBegin) / 3);
		}
		m_LODOffsets.emplace_back(std::distance(pBegin, pEnd));

		std::vector<LODNode> LOD1;
		for (uint64_t i = 0; i < LOD0.size(); i += 2)
		{

			const auto optimizedCount = GetOptimizedIndexCount(pBegin + LOD0[i + 0].m_Offset, LOD0[i + 0].m_IndexCount + LOD0[i + 1].m_IndexCount);
			LOD1.emplace_back(i * sizeof(LODNode), optimizedCount, static_cast<uint32_t>(i / 2));
		}

		XENON_LOG_INFORMATION("Something nice {}", LOD1.size());
	}

	LODTree::LODTree(uint32_t* pBegin, uint32_t* pEnd)
	{

	}
}