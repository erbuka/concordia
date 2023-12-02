#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

#include "texture.h"
#include "vecmath.h"


namespace ml
{
	class font
	{
	public:
		struct code_point
		{
			vec2f uv_top_left{};
			vec2f uv_bottom_right{};
			vec2f size{};
			vec2f offset{};
			float advance{};
			char character{};
		};

		float _ascent{};
		float _descent{};
		std::unordered_map<char, code_point> _code_points;

	private:
		texture2d _texture;
	public:
		
		static constexpr float sample_size = 128.0f;

		static font get_default_font();

		void load_from_file(const std::string_view path);
		void load_from_memory(const void* data, const std::size_t size);

		font() = default;

		font(const font&) = delete;
		font& operator=(const font&) = delete;

		font(font&&) = default;
		font& operator=(font&&) = default;

		[[nodiscard]] const texture2d& get_font_texture() const { return _texture; }

		[[nodiscard]] float get_string_width(const std::string_view string) const;

		[[nodiscard]] std::string shrink_to_fit(const std::string_view str, const float max_width) const;

		[[nodiscard]] float get_ascent() const { return _ascent; }
		[[nodiscard]] float get_descent() const { return _descent; }

		[[nodiscard]] const code_point& get_code_point(const char c) const { return _code_points.at(c); }

	};
}