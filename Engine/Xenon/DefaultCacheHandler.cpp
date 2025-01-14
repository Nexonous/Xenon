// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "DefaultCacheHandler.hpp"

#include "../XenonCore/Logging.hpp"

#include <fstream>

namespace Xenon
{
	std::vector<std::byte> DefaultCacheHandler::load(uint64_t hash)
	{
		std::vector<std::byte> cacheData;
		auto cacheFile = std::fstream(fmt::format("{}.bin", hash), std::ios::in | std::ios::binary | std::ios::ate);
		if (cacheFile.is_open())
		{
			const auto size = cacheFile.tellg();
			cacheFile.seekg(0);

			cacheData.resize(size);
			cacheFile.read(XENON_BIT_CAST(char*, cacheData.data()), size);

			cacheFile.close();
		}

		return cacheData;
	}

	void DefaultCacheHandler::store(uint64_t hash, const std::vector<std::byte>& bytes)
	{
		auto cacheFile = std::fstream(fmt::format("{}.bin", hash), std::ios::out | std::ios::binary);
		if (cacheFile.is_open())
		{
			cacheFile.write(XENON_BIT_CAST(const char*, bytes.data()), bytes.size());
			cacheFile.close();
		}
	}
}