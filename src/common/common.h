#pragma once

#include <cinttypes>
#include <cstdlib>
#include <array>
#include <mutex>

namespace cnc
{
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;


	using i8 = std::int8_t;
	using i16 = std::int16_t;
	using i32 = std::int32_t;

	using sample_t = i16;

	constexpr std::size_t audio_sample_rate = 44100;
	constexpr std::size_t audio_channels = 2;
	constexpr std::size_t buffer_size = 512 * audio_channels;
	constexpr std::size_t buffer_size_in_bytes = buffer_size * sizeof(sample_t);
	constexpr std::size_t max_queue_size_in_bytes = buffer_size * 5 * sizeof(sample_t);
	using buffer_t = std::array<std::int16_t, buffer_size>;

		
	template<typename T>
	class exclusive_resource
	{
	private:

		using mutex_t = std::mutex;
		using lock_t = std::unique_lock<mutex_t>;


		T _resource;
		mutex_t _mutex;
	public:
		template<typename Function> requires requires(Function f, T& t) { f(t); }
		auto use(Function&& f)
		{
			lock_t lock(_mutex);
			return (std::forward<Function>(f))(_resource);
		}

		std::pair<lock_t, T&> get()
		{
			return { std::unique_lock(_mutex), _resource };
		}
	};

}