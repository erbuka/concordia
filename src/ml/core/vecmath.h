#pragma once

#include <concepts>
#include <array>
#include <cmath>
#include <ranges>
#include <algorithm>
#include <stdexcept>

namespace ml
{

	namespace constants
	{
		template<std::floating_point T>
		constexpr T pi() { return static_cast<T>(3.141592653589793); }
	}

	template<std::floating_point T>
	T remap(const T value, const T from0, const T from1, const T to0, const T to1)
	{
		return (value - from0) / (from1 - from0) * (to1 - to0) + to0;
	}

	template<typename T> requires std::is_arithmetic_v<T>
	auto sign(const T v)
	{
		return v == static_cast<T>(0.0) ? 0 : v / std::abs(v);
	}


	template<typename T> requires std::is_arithmetic_v<T>
	constexpr auto radians(const T degrees) { return degrees * 0.0174532925f; }

	template<std::floating_point T, std::size_t N>
	constexpr bool almost_equal(const std::array<T, N>& a, const std::array<T, N>& b)
	{
		static constexpr T epsilon = T{ 0.0001 };

		for (std::size_t i = 0; i < N; ++i)
			if (std::abs(a[i] - b[i]) > epsilon)
				return false;
		return true;
	}




	template<typename T>
	class clamped_value
	{
	private:
		T _min{}, _max{}, _current{};

		static constexpr const T& clamp(const T& value, const T& min, const T& max) 
		{
			if(std::less{}(value, min)) return min;
			else if (std::less{}(max, value)) return max;
			else return value;
		}

	public:
		constexpr clamped_value() = default;
		constexpr clamped_value(const T& min, const T& max) : _min(min), _max(max) {}

		constexpr clamped_value& operator=(const T& value) { _current = clamp(value, _min, _max); return *this; }
		constexpr clamped_value& operator+=(const T& value) { _current = clamp(_current + value, _min, _max); return *this; }
		constexpr clamped_value& operator-=(const T& value) { _current = clamp(_current - value, _min, _max); return *this; }
		constexpr clamped_value& operator*=(const T& value) { _current = clamp(_current * value, _min, _max); return *this; }

		const T& value() const { return _current; }

		constexpr T normalized() const { return (_current - _min) / (_max - _min); }

		constexpr const T& min() const { return _min; }
		constexpr const T& max() const { return _max; }

		constexpr bool is_min() const { return _current == _min; }
		constexpr bool is_max() const { return _current == _max; }

	};

	template<typename T, std::size_t N> requires std::is_arithmetic_v<T>
	struct vec
	{
		std::array<T, N> data{};

		constexpr vec() = default;
		constexpr vec(const T t) { std::ranges::fill(data, t); }

		template<typename... Args> requires (sizeof...(Args) == N) && (std::is_convertible_v<Args, T> && ...)
		constexpr vec(const Args&... args) : data{ static_cast<T>(args)... } {}

		constexpr explicit vec(const vec<T, N - 1>& v, T last) requires (N > 1) 
		{
			std::ranges::copy(v.data, data.begin());
			data[N - 1] = last; 
		}

		template<std::size_t M> requires (M > N)
		constexpr explicit vec(const vec<T, M>& v)
		{
			std::ranges::copy(v.data | std::views::take(N), data.begin());
		}

		template<typename V> requires std::convertible_to<V, T>
		constexpr explicit vec(const vec<V, N>& v)
		{
			std::ranges::copy(v.data, data.begin());
		}


		constexpr T length() const requires std::floating_point<T>
		{
			T result = 0;
			for (std::size_t i = 0; i < N; ++i)
				result += data[i] * data[i];
			return std::sqrt(result);
		}

		constexpr vec normalized() const requires std::floating_point<T> { return *this / length(); }

		constexpr vec perp() const requires (N == 2)  { return { -data[1], data[0] }; }

		constexpr T product() const
		{
			T result = 1;
			for (std::size_t i = 0; i < N; ++i)
				result *= data[i];
			return result;
		}

		constexpr vec operator-() const { return *this * -1; }

		constexpr T& operator[](std::size_t i)  { return data[i]; }
		constexpr const T& operator[](std::size_t i) const { return data[i]; }
	};


	using vec1f = vec<float, 1>;
	using vec2f = vec<float, 2>;
	using vec3f = vec<float, 3>;
	using vec4f = vec<float, 4>;

	using vec1d = vec<double, 1>;
	using vec2d = vec<double, 2>;
	using vec3d = vec<double, 3>;
	using vec4d = vec<double, 4>;

	using vec1i = vec<std::int32_t, 1>;
	using vec2i = vec<std::int32_t, 2>;
	using vec3i = vec<std::int32_t, 3>;
	using vec4i = vec<std::int32_t, 4>;


	template<typename T>
	concept matrix = requires(T m)
	{
		typename T::value_type;
		typename T::index_type;
		{ T::row_count() } -> std::same_as<std::size_t>;
		{ T::col_count() } -> std::same_as<std::size_t>;
		{ m.operator[](typename T::index_type{ 0, 0 }) } -> std::convertible_to<typename T::value_type>;
	};

	template<typename T>
	concept square_matrix = matrix<T> && (T::row_count() == T::col_count());

	template<typename T, std::size_t N, std::size_t M>
	struct mat
	{
		using index_type = vec<std::size_t, 2>;
		using value_type = T;

		std::array<T, N* M> data;
		constexpr mat() { std::ranges::fill(data, T{ 0 }); }
		
		constexpr T& operator[](const index_type& i) { return data[i[0] * M + i[1]]; }
		constexpr const T& operator[](const index_type& i) const { return data[i[0] * M + i[1]]; }
		
		constexpr T& operator[](std::size_t i) { return data[i]; }	
		constexpr const T& operator[](std::size_t i) const { return data[i]; }

		constexpr T* ptr() { return data.data(); }
		constexpr T const* ptr() const { return data.data(); }

		static constexpr auto row_count() { return N; }
		static constexpr auto col_count() { return M; }
	};

	template<typename T, std::size_t N>
	using sqmat = mat<T, N, N>;

	using mat3f = sqmat<float, 3>;
	using mat4f = sqmat<float, 4>;

	template<std::floating_point T, std::size_t N>
	constexpr bool almost_equal(const vec<T, N>& a, const vec<T, N>& b) { return almost_equal(a.data, b.data); }

	template<std::floating_point T, std::size_t N, std::size_t M>
	constexpr bool almost_equal(const mat<T, N, M>& a, const mat<T, N, M>& b) { return almost_equal(a.data, b.data); }

	template<typename T, std::size_t N>
	T dot(const vec<T, N>& lhs, const vec<T, N>& rhs)
	{
		T result = 0;
		for (std::size_t i = 0; i < N; ++i)
			result += lhs.data[i] * rhs.data[i];
		return result;
	}
	
	template<typename T, std::size_t N>
	constexpr bool operator==(const vec<T, N>& lhs, const vec<T, N>& rhs)
	{
		for (std::size_t i = 0; i < N; ++i)
			if (lhs.data[i] != rhs.data[i])
				return false;
		return true;
	}

	template<typename T, std::size_t N>
	constexpr vec<T, N> operator+(const vec<T, N>& lhs, const vec<T, N>& rhs)
	{
		vec<T, N> result{};
		for (std::size_t i = 0; i < N; ++i)
			result.data[i] = lhs.data[i] + rhs.data[i];
		return result;
	}

	template<typename T, std::size_t N>
	constexpr vec<T, N> operator-(const vec<T, N>& lhs, const vec<T, N>& rhs)
	{
		vec<T, N> result{};
		for (std::size_t i = 0; i < N; ++i)
			result.data[i] = lhs.data[i] - rhs.data[i];
		return result;
	}

	template<typename T, std::size_t N>
	constexpr vec<T, N> operator*(const vec<T, N>& lhs, const vec<T, N>& rhs)
	{
		vec<T, N> result{};
		for (std::size_t i = 0; i < N; ++i)
			result.data[i] = lhs.data[i] * rhs.data[i];
		return result;
	}

	template<typename T, typename K, std::size_t N> requires requires(T t, K k) { t * k; }
	constexpr vec<T, N> operator*(const vec<T, N>& lhs, const K rhs)
	{
		vec<T, N> result{};
		for (std::size_t i = 0; i < N; ++i)
			result.data[i] = lhs.data[i] * rhs;
		return result;
	}

	template<typename T, typename K, std::size_t N> requires requires(T t, K k) { t / k; }
	constexpr vec<T, N> operator/(const vec<T, N>& lhs, const K rhs)
	{
		vec<T, N> result{};
		for (std::size_t i = 0; i < N; ++i)
			result.data[i] = lhs.data[i] / rhs;
		return result;
	}


	template<typename T, std::size_t N>
	constexpr vec<T, N> operator/(const vec<T, N>& lhs, const vec<T, N>& rhs)
	{
		vec<T, N> result{};
		for (std::size_t i = 0; i < N; ++i)
			result.data[i] = lhs.data[i] / rhs.data[i];
		return result;
	}

	template<typename T, std::size_t N>
	constexpr vec<T, N> lerp(const vec<T, N>& a, const vec<T, N>& b, std::floating_point auto const t)
	{
		return a + (b - a) * t;
	}

	template<typename T, std::size_t N, std::size_t M>
	constexpr vec<T, N> operator*(const mat<T, N, M>& lhs, const vec<T, M>& rhs)
	{
		vec<T, N> result{};
		for (std::size_t i = 0; i < N; ++i)
			for (std::size_t j = 0; j < M; ++j)
				result.data[i] += lhs[{i, j}] * rhs.data[j];
		return result;
	}

	template<typename T, std::size_t N, std::size_t M, std::size_t Q>
	constexpr mat<T, N, Q> operator*(const mat<T, N, M>& lhs, const mat<T, M, Q>& rhs)
	{
		mat<T, N, Q> result{};
		for (std::size_t i = 0; i < N; ++i)
			for (std::size_t j = 0; j < Q; ++j)
				for (std::size_t k = 0; k < M; ++k)
					result[{i, j}] += lhs[{i, k}] * rhs[{k, j}];
		return result;
	}


	template<typename T, std::size_t N, std::size_t M>
	constexpr bool operator==(const mat<T, N, M>& lhs, const mat<T, N, M>& rhs)
	{
		for (std::size_t i = 0; i < N; ++i)
			for (std::size_t j = 0; j < M; ++j)
				if (lhs[{i, j}] != rhs[{i, j}])
					return false;
		return true;
	}

	template<std::size_t CPN, typename T, std::size_t N>
	struct bezier_curve 
	{
		std::array<vec<T, N>, CPN> control_points;

		constexpr auto eval(std::floating_point auto const t) const
		{
			std::array<vec<T, N>, CPN> points = control_points;
			for (std::size_t i = 1; i < CPN; ++i)
				for (std::size_t j = 0; j < CPN - i; ++j)
					points[j] = lerp(points[j], points[j + 1], t);
			return points[0];
		}

	};


	using bezier_qudratic_2f = bezier_curve<3, float, 2>;
	using bezier_cubic_2f = bezier_curve<4, float, 2>;


	template<square_matrix M>
	constexpr M identity()
	{
		M result{};
		for(std::size_t i = 0; i < M::row_count(); ++i)
			result[{i, i}] = 1;
		return result;
	}

	template<std::floating_point T> 
	constexpr sqmat<T, 4> ortho(const T left, const T right, const T bottom, const T top, const T near = 0, const T far = 100)
	{
		sqmat<T, 4> result = identity<sqmat<T, 4>>();
		result[{0, 0}] = 2 / (right - left);
		result[{1, 1}] = 2 / (top - bottom);
		result[{2, 2}] = -2 / (far - near);
		result[{0, 3}] = -(right + left) / (right - left);
		result[{1, 3}] = -(top + bottom) / (top - bottom);
		result[{2, 3}] = -(far + near) / (far - near);
		return result;
	}

	template<std::floating_point T>
	constexpr sqmat<T, 4> get_translation(const vec<T, 3>& t)
	{
		sqmat<T, 4> result = identity<sqmat<T, 4>>();
		result[{0, 3}] = t[0];
		result[{1, 3}] = t[1];
		result[{2, 3}] = t[2];
		return result;
	}

	template<std::floating_point T>
	constexpr sqmat<T, 4> get_scale(const vec<T, 3>& s)
	{
		sqmat<T, 4> result = identity<sqmat<T, 4>>();
		result[{0, 0}] = s[0];
		result[{1, 1}] = s[1];
		result[{2, 2}] = s[2];
		return result;
	}
	
	template<std::floating_point T>
	constexpr sqmat<T, 4> get_rotation(const vec<T, 3>& axis, const T angle)
	{
		sqmat<T, 4> result = identity<sqmat<T, 4>>();
		const T c = std::cos(angle);
		const T s = std::sin(angle);
		const T t = 1 - c;
		const T x = axis[0];
		const T y = axis[1];
		const T z = axis[2];
		result[{0, 0}] = t * x * x + c;
		result[{0, 1}] = t * x * y + s * z;
		result[{0, 2}] = t * x * z - s * y;
		result[{1, 0}] = t * x * y - s * z;
		result[{1, 1}] = t * y * y + c;
		result[{1, 2}] = t * y * z + s * x;
		result[{2, 0}] = t * x * z + s * y;
		result[{2, 1}] = t * y * z - s * x;
		result[{2, 2}] = t * z * z + c;
		return result;
	}

	template<std::floating_point T>
	constexpr sqmat<T, 4> get_inverse(const sqmat<T, 4>& m)
	{
		sqmat<T, 4> inv, invOut;

		inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
		inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
		inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
		inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
		inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
		inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
		inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
		inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
		inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
		inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
		inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
		inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
		inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
		inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
		inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
		inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

		float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

		if (det == 0)
			throw std::runtime_error("Determinant is zero");

		det = 1.0 / det;

		for (std::size_t i = 0; i < 16; i++)
			invOut[i] = inv[i] * det;

		return invOut;

	}



}