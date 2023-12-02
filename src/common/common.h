#pragma once

#include <cinttypes>
#include <cstdlib>
#include <array>

namespace cnc
{
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;


	using i8 = std::int8_t;
	using i16 = std::int16_t;
	using i32 = std::int32_t;

	using sample_t = i16;

	constexpr std::size_t buffer_size = 1024;
	constexpr std::size_t buffer_size_in_bytes = buffer_size * sizeof(sample_t);
	constexpr std::size_t max_queue_size_in_bytes = buffer_size * 5 * sizeof(sample_t);
	using buffer_t = std::array<std::int16_t, buffer_size>;


}