// Copyright 2022-2023 Dhiraj Wishal
// SPDX-License-Identifier: Apache-2.0

#include "Logging.hpp"
#include "../XenonCore/Common.hpp"

#include <array>

namespace Xenon
{
	void HexDump(const std::byte* pBegin, const std::byte* pEnd)
	{
		constexpr auto rowSize = 16;

		const auto size = std::distance(pBegin, pEnd);
		const auto pointerInteger = XENON_BIT_CAST(std::ptrdiff_t, pBegin);

		// Calculate the number of rows.
		const uint32_t rows = std::ceil(static_cast<float>(size) / rowSize);

		// Iterate over the rows and print the data to the stream.
		for (uint32_t i = 0; i < rows; i++)
		{
			const auto rowIndex = i * rowSize;

			// Insert the data and also build the byte line string.
			std::array<uint8_t, rowSize> bytes{ 0 };
			std::array<char, rowSize> byteBuffer{ '.' };

			bytes.fill(0);
			byteBuffer.fill('.');

			for (uint32_t j = 0; j < rowSize; j++)
			{
				const auto index = rowIndex + j;

				// If we are within the bounds, process the byte.
				if (index < size)
				{
					const auto value = static_cast<const int>(pBegin[index]);
					bytes[j] = value;

					if (std::isalpha(value) || std::ispunct(value) || std::isspace(value))
						byteBuffer[j] = value;
				}
			}

			const auto currentOffset = pointerInteger + rowIndex;

			// Finally, print the line.
			XENON_LOG_INFORMATION("[0x{:016X} ... 0x{:016X}] "
				" {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X}  {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} {:02X} "
				" {} {} {} {} {} {} {} {}  {} {} {} {} {} {} {} {} ",
				currentOffset, currentOffset + rowSize - 1,
				bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
				bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15],
				byteBuffer[0], byteBuffer[1], byteBuffer[2], byteBuffer[3], byteBuffer[4], byteBuffer[5], byteBuffer[6], byteBuffer[7],
				byteBuffer[8], byteBuffer[9], byteBuffer[10], byteBuffer[11], byteBuffer[12], byteBuffer[13], byteBuffer[14], byteBuffer[15]);
		}
	}
}