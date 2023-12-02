#pragma once

#include <semaphore>
#include <vector>
#include <string>
#include <variant>
#include <algorithm>
#include <type_traits>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <list>
#include <cctype>

#include "vecmath.h"


namespace ml
{
	
	template<typename T>
	using simple_delete = decltype([](T* ptr) { delete ptr; });

	template<typename T, typename Param, typename Result>
	concept has_call_operator = requires (T t, Param p) {
		{ t.operator()(p) } -> std::same_as<std::remove_cvref_t<Result>>;
	};


	template<typename... Args>
	struct type_list
	{
		template<typename T>
		static constexpr bool contains = (std::is_same_v<T, Args> || ...);

		using variant_t = std::variant<Args...>;

		template<typename Result, typename Visitor, typename Variant> 
			requires (has_call_operator<Visitor, Args, Result> && ...) && std::same_as<std::remove_cvref_t<Variant>, variant_t>
		static Result visit(Variant&& var, Visitor&& vis)
		{
			return std::visit([&](auto&& arg) { return vis(std::forward<decltype(arg)>(arg)); }, std::forward<Variant>(var));
		}

	};

	template<typename T>
	auto to_pointer(T& value)
	{
		if(std::is_pointer_v<T>) return value;
		else return std::addressof(value);
	}

	template<typename... Ts>
	struct overloaded : Ts... {
		using Ts::operator()...;
	};


	template <typename Key, typename Value, std::size_t N>
	struct static_map 
	{
		using pair_type = std::pair<Key, Value>;
		using storage_type = std::array<pair_type, N>;
		storage_type data;
		constexpr auto& operator[](const Key& k) { return std::ranges::find_if(data, [&](const auto& p) { return p.first == k; })->second; }
		constexpr const auto& operator[](const Key& k) const { return std::ranges::find_if(data, [&](const auto& p) { return p.first == k; })->second;  }
		constexpr bool contains(const Key& k) const { return std::ranges::find_if(data, [&](const auto& p) { return p.first == k; }) != data.end(); }
		

	
	};

	template<typename Key, typename Value, typename... U>
	static_map(const std::pair<Key, Value>, U...) -> static_map<Key, Value, 1 + sizeof...(U)>;

	struct default_success {};
	struct default_error {};

	template<typename T = default_error>
	struct unexpected
	{
		const T value;
		unexpected() = default;
		unexpected(const T& v) : value(v) {}
		unexpected(T&& v) : value(std::move(v)) {}
	};

	template<typename T = default_success, typename E = default_error>
	class expected
	{
	private:
		std::variant<T, E> _value = E{};
	public:

		expected(const T& value) : _value(value) {}
		expected(T&& value) : _value(std::move(value)) {}

		expected(unexpected<E>&& e) : _value(std::move(e.value)) {}
		expected(const unexpected<E>& e) : _value(value) {}

		bool is_error() const { return std::holds_alternative<E>(_value); }
		bool has_value() const { return std::holds_alternative<T>(_value); }
		const T& value() const { return std::get<T>(_value); }
		const E& error() const { return std::get<E>(_value); }
	};


	constexpr auto trim_left = std::views::drop_while([](const char ch) { return std::isspace(ch); });
	constexpr auto trim_right = std::views::reverse | trim_left | std::views::reverse;
	constexpr auto trim = trim_left | trim_right;

	//std::string_view trim(std::string_view str);
	std::string to_lower(const std::string_view str);
	std::size_t count_words(std::string_view str);
	std::list<std::string> split(const std::string_view str, const std::string_view delim);
	
	namespace ranges
	{
		template<std::ranges::input_range R>
		bool contains(R&& r, const auto& value)
		{
			return std::ranges::find(std::forward<R>(r), value) != r.end();
 		}

		auto enumerate = std::views::transform([i = std::size_t(0)]<typename T>(T&& val) mutable {
			return std::pair<std::size_t, T>(i++, std::forward<T>(val));
		});

		template<
			template<typename> typename Container, 
			std::ranges::input_range R,
			typename ValueType = std::ranges::range_value_t<R>
		>
		auto to(R&& r)
		{
			Container<ValueType> c;

			if constexpr (requires (R & r) { std::back_inserter(c); })
			{
				std::ranges::copy(std::forward<R>(r), std::back_inserter(c));
				return c;
			}
			else throw std::runtime_error("to: Container does not support back_inserter");

		}

	}



}