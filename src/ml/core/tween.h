#pragma once

#include <cassert>
#include <string_view>
#include <vector>
#include <utility>
#include <tuple>
#include <algorithm>

namespace ml
{

	namespace easing
	{
		static constexpr float linear(const float t) { return t; }
	}

	template<typename... Args>
		requires ((requires (Args t) { t * 1.0f; t + t; }) && ...)
	struct tween_tuple 
	{
		std::tuple<Args...> data;
		tween_tuple() = default;
		tween_tuple(Args... args) : data{ args... } {}

		template<std::size_t N>
		auto& get() { return std::get<N>(data); }

	};


	template<typename... Args>
	bool operator==(const tween_tuple<Args...>& lhs, const tween_tuple<Args...>& rhs)
	{
		return lhs.data == rhs.data;
	}

	template<typename... Args, typename T> requires std::is_arithmetic_v<T>
	tween_tuple<Args...> operator*(const tween_tuple<Args...>& lhs, const T k)
	{
		tween_tuple<Args...> result;
		[&] <std::size_t... idx>(std::index_sequence<idx...>) {
			((std::get<idx>(result.data) = std::get<idx>(lhs.data) * k), ...);
		}(std::make_index_sequence<sizeof...(Args)>());
		return result;
	}

	template<typename... Args>
	tween_tuple<Args...> operator+(const tween_tuple<Args...>& lhs, const tween_tuple<Args...>& rhs)
	{
		tween_tuple<Args...> result;
		[&]<std::size_t... idx>(std::index_sequence<idx...>) {
			((std::get<idx>(result.data) = std::get<idx>(lhs.data) + std::get<idx>(rhs.data)), ...);
		}(std::make_index_sequence<sizeof...(Args)>());
		return result;
	}


	template<typename T> 
	struct tween_animation
	{
		std::string_view name;
		T start{};
		T end{};
		float duration{};
		bool auto_next{ false };
	};

	template<typename T> 
		requires std::is_copy_assignable_v<T> && requires (T t) { t * 1.0f; t + t; }
	class tween
	{
	private:
		T _value{};
		std::size_t _current{};
		std::vector<tween_animation<T>> _animations;
		float _time{};
	public:
		tween() = default;
		tween(std::initializer_list<tween_animation<T>> anims) :
			_animations(anims.begin(), anims.end()) 
		{
			set_current(_animations.begin()->name);
		}

		T value() const { return _value; }

		bool is(const std::string_view name) const { return _animations[_current].name == name; }

		std::string_view get_current() const { return _animations[_current].name; }
		void set_current(const std::string_view name)
		{
			auto it = std::ranges::find_if(_animations, [&](const auto& anim) { return anim.name == name; });
			assert(it != _animations.end());
			_time = 0.0f;
			_current = std::distance(_animations.begin(), it);
			_value = _animations[_current].start;
		}

		void update(const float dt)
		{
			if (_animations.empty())
				return;

			auto& anim = _animations[_current];
			
			if (anim.duration > 0.0f)
			{
				_time = std::clamp(_time + dt, 0.0f, _animations[_current].duration);
				const auto t = _time / anim.duration;
				_value = anim.start * (1 - t) + anim.end * t;
			}
			else _value = anim.end;

			if (has_ended() && anim.auto_next)
				set_current(_animations[(_current + 1) % _animations.size()].name);

		}

		void end() { _time = _animations[_current].duration; }
		bool has_ended() const { return _time == _animations[_current].duration; }

	};

}