#pragma once

#include <cinttypes>
#include <cstdlib>
#include <array>
#include <mutex>
#include <ranges>
#include <utility>
#include <algorithm>
#include <tuple>

namespace cnc
{
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;


	using i8 = std::int8_t;
	using i16 = std::int16_t;
	using i32 = std::int32_t;

	using sample_t = i16;

	constexpr std::size_t audio_sample_rate = 16000;
	constexpr std::size_t audio_channels = 1;
	constexpr std::size_t buffer_size = 512 * audio_channels;
	constexpr std::size_t buffer_size_in_bytes = buffer_size * sizeof(sample_t);
	constexpr std::size_t max_queue_size_in_bytes = buffer_size * 5 * sizeof(sample_t);
	using buffer_t = std::array<std::int16_t, buffer_size>;

	template<typename K, typename V, std::size_t N>
	class static_map
	{
	public:
		std::array<std::pair<K, V>, N> data;

		constexpr const V& operator[](const K& k) const
		{
			const auto it = std::ranges::find_if(data, [&](const auto& p) { return p.first == k; });
			if (it == data.end())
				throw std::runtime_error("Not found");
			return it->second;
		}

	};

	class history_buffer
	{
	private:
		static constexpr float history_length_in_seconds{ 0.25f };
		static constexpr std::size_t buffer_count{ static_cast<std::size_t>(audio_sample_rate * history_length_in_seconds) / buffer_size };
		static constexpr std::size_t size_in_samples{ buffer_size * buffer_count };
		std::size_t _current_idx{ 0 };
		std::size_t _insert_count{ 0 };
		std::array<sample_t, size_in_samples> _buffer{};

		std::size_t wrap(const std::size_t i) const { return i % size_in_samples; }

	public:
		using value_type = sample_t;

		class iterator_sentinel {};

		class iterator
		{
		private:
			history_buffer& _owner;
			std::size_t _start, _offset;
		public:
			iterator(history_buffer& owner, const std::size_t start, const std::size_t offset) :
				_start(start), _owner(owner), _offset(offset) {}

			sample_t& operator*() { return _owner.get(_start + _offset); }

			iterator& operator++() {
				++_offset;
				return *this;
			}

			bool operator==(iterator_sentinel) const { return _offset == size_in_samples; }
			bool operator!=(iterator_sentinel) const { return _offset != size_in_samples; }

		};

		sample_t& operator[](const std::size_t idx) { return get(idx); }
		const sample_t& operator[](const std::size_t idx) const { return get(idx); }

		sample_t& get(const std::size_t idx) { return _buffer[wrap(idx + _current_idx)]; }
		const sample_t& get(const std::size_t idx) const { return _buffer[wrap(idx + _current_idx)]; }

		auto begin() { return iterator{ *this, _current_idx, 0 }; }
		auto end() { return iterator_sentinel{}; }

		std::size_t collect_inserted_bytes()
		{
			const auto val = std::exchange(_insert_count, 0);
			return val * sizeof(sample_t);
		}

		void push_back(const sample_t s)
		{
			get(0) = s;
			_current_idx = wrap(_current_idx + 1);
			_insert_count++;
		}

		sample_t sample(const float age) const
		{
			/*
			const std::size_t start = ((_current_idx / buffer_size + 1) * buffer_size) % size_in_samples;
			return get(start + static_cast<std::size_t>(age * size_in_samples));
			*/
			return get(size_in_samples * age);
		}

	};

		
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