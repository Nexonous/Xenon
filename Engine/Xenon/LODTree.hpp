// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../XenonCore/Common.hpp"

#include <vector>

namespace Xenon
{
	/**
	 * LOD Tree structure.
	 * This is a tree structure that contains information about multiple LODs. Each LOD is stored as offsets.
	 *
	 * Connecting system:
	 * - Triangles are grouped together when they share a common edge.
	 * - Two triangles make up one node.
	 * - Each LOD is made up of multiple nodes covering the surface.
	 * - If we cannot merge or simplify triangles when merging two nodes, keep them together and create the new node. This way we can simplify them in lower LOD levels.
	 * - Each LOD is made up of triangles. Triangle information are stored using indices.
	 *
	 * Generation steps:
	 * - Generate LOD0 by clustering two triangles together to generate a single node.
	 * - Recursively apply the same process for other LODs until we can't produce anymore, or found a mathematical solution to calculate the maximum LOD depth.
	 */
	class LODTree final
	{
	public:
		/**
		 * Default constructor.
		 */
		LODTree() = default;

		/**
		 * Explicit constructor.
		 * This generates the tree using the indices as uint16_t elements.
		 *
		 * @param pBegin The beginning point to the data.
		 * @param pEnd The end pointer to the data.
		 */
		explicit LODTree(uint16_t* pBegin, uint16_t* pEnd);

		/**
		 * Explicit constructor.
		 * This generates the tree using the indices as uint32_t elements.
		 *
		 * @param pBegin The beginning point to the data.
		 * @param pEnd The end pointer to the data.
		 */
		explicit LODTree(uint32_t* pBegin, uint32_t* pEnd);

	private:
		std::vector<uint64_t> m_LODOffsets;
	};
}
