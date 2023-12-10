#pragma once

#include <memory>
#include <vector>
#include <string_view>

#include "vecmath.h"

namespace ml
{

	enum class texture_filter_mode
	{
		none,
		nearest,
		linear,
		linear_mipmap_linear
	};

	enum class texture_format
	{
		none,
		rgba8,
		rgba32f,
		rgb8,
		rgb32f,
		r8,
		r16f
	};

	struct sprite2d;


	class texture2d
	{
	private:
		std::uint32_t _width, _height;
		std::uint32_t _id{};
		texture_filter_mode _filter_mode{ texture_filter_mode::none };
		texture_format _format { texture_format::none };
	public:

		texture2d() = default;
		texture2d(const texture2d&) = delete;
		texture2d& operator=(const texture2d&) = delete;

		texture2d(texture2d&&);
		texture2d& operator=(texture2d&&);

		~texture2d();
	
		static texture2d load(const texture_format fmt, const std::size_t width, const std::size_t height, const texture_filter_mode filter, const void* data);
		static texture2d load_from_file(const std::string_view path, const texture_format fmt, const  texture_filter_mode filter);
		static texture2d load_from_memory(const void* data, const std::size_t length, const texture_format fmt, const  texture_filter_mode filter);

		std::uint32_t get_native_handle() const;

		texture_filter_mode get_filter_mode() const { return _filter_mode; }

		texture_format get_format() const { return _format; }

		void upload(const void* data);

		std::uint32_t get_width() const { return _width; }
		std::uint32_t get_height() const { return _height; }

		std::vector<sprite2d> slice_all(const std::uint32_t sx, const std::uint32_t sy) const;
		sprite2d slice(const std::uint32_t sx, const std::uint32_t sy, const std::uint32_t x, const std::uint32_t y) const;

	};

	struct sprite2d
	{
		const texture2d& tex;
		const vec2f uv_bottom_left, uv_top_right;

		sprite2d() = default;
		sprite2d(const texture2d& tx, const vec2f& bl, const vec2f& tr) :
			tex(tx), uv_bottom_left(bl), uv_top_right(tr) {}


	};
	
	class framebuffer
	{
	private:
		std::vector<texture_format> _attachment_formats;
		std::vector<texture2d> _attachments;
		std::uint32_t _id{};
	public:
		framebuffer() = default;
		
		framebuffer(const framebuffer&) = delete;
		framebuffer& operator=(const framebuffer&) = delete;

		framebuffer(framebuffer&&);
		framebuffer& operator=(framebuffer&&);

		~framebuffer();


		texture2d& get_attachment(const std::size_t index);
		const texture2d& get_attachment(const std::size_t index) const;

		void create(std::initializer_list<texture_format> attachments, const std::int32_t width, const std::int32_t height);
		void resize(const std::int32_t width, const std::int32_t height);
		std::uint32_t get_native_handle() const { return _id; }
		bool check_complete() const;
		void bind(const int32_t level = 0) const;
		
		static void unbind();

	};

}