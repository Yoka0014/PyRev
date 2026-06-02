#pragma once

#include <cstdint>

#include "../config.h"
#include "array.h"
#include "unroller.h"

#if defined(USE_SSE42) 

#ifdef USE_X64
#define CRC32(crc, d) _mm_crc32_u64((crc),(d))
#else
#define CRC32(crc, d) _mm_crc32_u32(_mm_crc32_u32((crc),(d)),((d)>>32))
#endif

#else

constexpr utils::Array<uint32_t, 256> CRC32_TABLE(
	[](uint32_t* data, size_t len)
	{
		constexpr uint32_t kPoly = 0x82F63B78u;
		for (uint32_t i = 0; i < 256; ++i)
		{
			uint32_t c = i;
			for (int bit = 0; bit < 8; ++bit)
			{
				c = (c & 1u) ? (c >> 1) ^ kPoly : (c >> 1);
			}
			data[i] = c;
		}
	});

inline uint32_t crc32_u64(uint32_t crc, uint64_t data)
{
	utils::LoopUnroller<8>()([&](const int32_t)
	{
		uint32_t idx = (crc ^ static_cast<uint8_t>(data & 0xFFu)) & 0xFFu;
		crc = (crc >> 8) ^ CRC32_TABLE[idx];
		data >>= 8;
	});
	return crc;
}

#define CRC32(crc, d) crc32_u64((crc), (d))

#endif