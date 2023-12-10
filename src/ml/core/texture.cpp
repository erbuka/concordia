#include "texture.h"

#include <tuple>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>

#include "application.h"
#include "utility.h"
#include "log.h"

namespace ml
{

	static constexpr static_map s_formats = {
		std::pair{ texture_format::rgb8, GL_RGB8 },
		std::pair{ texture_format::rgba8, GL_RGBA8 },
		std::pair{ texture_format::rgb32f, GL_RGB32F },
		std::pair{ texture_format::rgba32f, GL_RGBA32F },
		std::pair{ texture_format::r16f, GL_R16F },
		std::pair{ texture_format::r8, GL_R8 }
	};

	static constexpr static_map s_upload_formats = {
		std::pair { texture_format::rgb8, std::pair{ GL_RGB, GL_UNSIGNED_BYTE } },
		std::pair { texture_format::rgba8, std::pair{ GL_RGBA, GL_UNSIGNED_BYTE } },
		std::pair { texture_format::rgb32f, std::pair{ GL_RGB, GL_FLOAT } },
		std::pair { texture_format::rgba32f, std::pair{ GL_RGBA, GL_FLOAT } },
		std::pair { texture_format::r16f, std::pair{ GL_RED, GL_FLOAT } },
		std::pair { texture_format::r8, std::pair{ GL_RED, GL_UNSIGNED_BYTE } }
	};

	static constexpr static_map s_filter_modes = {
		std::pair{ texture_filter_mode::nearest, std::pair{ GL_NEAREST, GL_NEAREST } },
		std::pair{ texture_filter_mode::linear, std::pair{ GL_LINEAR, GL_LINEAR } },
		std::pair{ texture_filter_mode::linear_mipmap_linear, std::pair { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR } }
	};

	static std::size_t get_mipmap_levels(const std::size_t width, const std::size_t height)
	{
		return static_cast<std::size_t>(std::log2(std::max(width, height))) + 1;
	}


	texture2d texture2d::load(const texture_format fmt, const std::size_t width, const std::size_t height, const  texture_filter_mode mode, const void* data)
	{
		texture2d tex;

		const auto [layout, type] = s_upload_formats[fmt];

		const auto mipmap_levels = mode == texture_filter_mode::linear_mipmap_linear ?
			get_mipmap_levels(width, height) : 1;

		glCreateTextures(GL_TEXTURE_2D, 1, &tex._id);
		glBindTexture(GL_TEXTURE_2D, tex._id);
		glTexStorage2D(GL_TEXTURE_2D, mipmap_levels, s_formats[fmt], width, height);

		if(data)
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, layout, type, data);

		if (mode == texture_filter_mode::linear_mipmap_linear)
			glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, s_filter_modes[mode].first);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, s_filter_modes[mode].second);

		tex._format = fmt;
		tex._filter_mode = mode;
		tex._width = width;
		tex._height = height;

		return tex;
	}

	texture2d texture2d::load_from_memory(const void* ptr, const std::size_t length, const texture_format fmt, const texture_filter_mode mode)
	{
		std::int32_t width, height, channels;

		stbi_set_flip_vertically_on_load(true);

		std::unique_ptr<float, decltype([](float* p) { stbi_image_free(p); }) > 
			data(stbi_loadf_from_memory(static_cast<const std::uint8_t*>(ptr), length, &width, &height, &channels, 4));

		if (data == nullptr)
			throw std::runtime_error("Something went very wrong!");

		texture2d tex;
		glCreateTextures(GL_TEXTURE_2D, 1, &tex._id);
		glBindTexture(GL_TEXTURE_2D, tex._id);
		glTexStorage2D(GL_TEXTURE_2D, get_mipmap_levels(width, height), s_formats[fmt], width, height);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data.get());

		if (mode == texture_filter_mode::linear_mipmap_linear)
			glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, tex._id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, s_filter_modes[mode].first);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, s_filter_modes[mode].second);

		tex._format = fmt;
		tex._filter_mode = mode;
		tex._width = width;
		tex._height = height;

		return tex;


	}

	texture2d texture2d::load_from_file(const std::string_view path, const texture_format fmt, const texture_filter_mode mode)
	{
		namespace fs = std::filesystem;

		if (!fs::is_regular_file(path))
			throw std::runtime_error(std::format("File not found: {}", path));

		std::int32_t width, height, channels;

		stbi_set_flip_vertically_on_load(true);

		std::unique_ptr<float, decltype([](float* ptr) { stbi_image_free(ptr); }) > data(stbi_loadf(path.data(), &width, &height, &channels, 4));

		if (data == nullptr)
			throw std::runtime_error("Something went very wrong!");

		texture2d tex;
		glCreateTextures(GL_TEXTURE_2D, 1, &tex._id);
		glBindTexture(GL_TEXTURE_2D, tex._id);
		glTexStorage2D(GL_TEXTURE_2D, get_mipmap_levels(width, height), s_formats[fmt], width, height);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data.get());

		if (mode == texture_filter_mode::linear_mipmap_linear)
			glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, tex._id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, s_filter_modes[mode].first);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, s_filter_modes[mode].second);

		tex._format = fmt;
		tex._filter_mode = mode;
		tex._width = width;
		tex._height = height;

		return tex;

	}


	void texture2d::upload(const void* data)
	{
		const auto [layout, type] = s_upload_formats[_format];
		glBindTexture(GL_TEXTURE_2D, _id);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, layout, type, data);
	}

	std::vector<sprite2d> texture2d::slice_all(const std::uint32_t sx, const std::uint32_t sy) const
	{
		// Slice the texture into a grid of sprites
		// The sprites are returned in row-major order

		std::vector<sprite2d> ret;

		for (std::uint32_t y = 0; y < sy; ++y)
			for (std::uint32_t x = 0; x < sx; ++x)
				ret.push_back(slice(sx, sy, x, y));

		return ret;
	}
	sprite2d texture2d::slice(const std::uint32_t sx, const std::uint32_t sy, const std::uint32_t x, const std::uint32_t y) const
	{
		vec2f step{ 1.0f / sx, 1.0f / sy };
		return { *this ,
			step * vec2f{ x, y },
			step * vec2f{ x + 1, y + 1}
		};
	}

	texture2d::texture2d(texture2d&& other) 
	{
		_width = other._width;
		_height = other._height;
		_format = other._format;
		_filter_mode = other._filter_mode;
		std::swap(_id, other._id); 
	}

	texture2d& texture2d::operator=(texture2d&& other)
	{
		_width = other._width;
		_height = other._height;
		_format = other._format;
		_filter_mode = other._filter_mode;
		std::swap(_id, other._id);
		return *this;
	}

	texture2d::~texture2d()
	{
		if (_id)
		{
			TA_INFO("Texture deleted: {}", _id);
			glDeleteTextures(1, &_id);
		}
	}

	std::uint32_t texture2d::get_native_handle() const { return _id; }

#pragma region framebuffer

	framebuffer::framebuffer(framebuffer&& other) { std::swap(_id, other._id); }

	framebuffer::~framebuffer() { if (_id) glDeleteFramebuffers(1, &_id); }

	framebuffer& framebuffer::operator=(framebuffer&& other)
	{
		std::swap(_id, other._id);
		return *this;
	}

	void framebuffer::create(std::initializer_list<texture_format> attachment_formats, const std::int32_t width, const std::int32_t height)
	{
		// TODO: check if the framebuffer is already created
		std::ranges::copy(attachment_formats, std::back_inserter(_attachment_formats));
		_attachments.reserve(attachment_formats.size());
		glCreateFramebuffers(1, &_id);
		resize(width, height);
	}

	void framebuffer::resize(const std::int32_t width, const std::int32_t height)
	{
		std::int32_t cw, ch;
		glGetNamedFramebufferParameteriv(_id, GL_FRAMEBUFFER_DEFAULT_WIDTH, &cw);
		glGetNamedFramebufferParameteriv(_id, GL_FRAMEBUFFER_DEFAULT_HEIGHT, &ch);

		if(cw == width && ch == height)
			return;

		glNamedFramebufferParameteri(_id, GL_FRAMEBUFFER_DEFAULT_WIDTH, width);
		glNamedFramebufferParameteri(_id, GL_FRAMEBUFFER_DEFAULT_HEIGHT, height);

		// This should destroy the previous textures
		_attachments.clear();
		
		for (auto fmt : _attachment_formats)
		{
			auto tex = texture2d::load(fmt, width, height, texture_filter_mode::linear, nullptr);
			_attachments.push_back(std::move(tex));
		}

	}

	bool framebuffer::check_complete() const
	{
		return glCheckNamedFramebufferStatus(_id, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}

	void framebuffer::bind(const int32_t level) const
	{

		static constexpr std::array<GLenum, 16> s_color_attachments = {
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,
			GL_COLOR_ATTACHMENT4,
			GL_COLOR_ATTACHMENT5,
			GL_COLOR_ATTACHMENT6,
			GL_COLOR_ATTACHMENT7,
			GL_COLOR_ATTACHMENT8,
			GL_COLOR_ATTACHMENT9,
			GL_COLOR_ATTACHMENT10,
			GL_COLOR_ATTACHMENT11,
			GL_COLOR_ATTACHMENT12,
			GL_COLOR_ATTACHMENT13,
			GL_COLOR_ATTACHMENT14,
			GL_COLOR_ATTACHMENT15,
		};

		std::int32_t width, height;
		glGetNamedFramebufferParameteriv(_id, GL_FRAMEBUFFER_DEFAULT_WIDTH, &width);
		glGetNamedFramebufferParameteriv(_id, GL_FRAMEBUFFER_DEFAULT_HEIGHT, &height);

		width /= 1 << level;
		height /= 1 << level;


		for (auto i = 0u; i < _attachment_formats.size(); ++i)
			glNamedFramebufferTexture(_id, s_color_attachments[i], _attachments[i].get_native_handle(), level);

		glNamedFramebufferDrawBuffers(_id, _attachment_formats.size(), s_color_attachments.data());

		glBindFramebuffer(GL_FRAMEBUFFER, _id);
		glViewport(0, 0, width, height);
	}

	void framebuffer::unbind()
	{
		const auto ws = app::get_window_size();
		glViewport(0, 0, ws[0], ws[1]);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	texture2d& framebuffer::get_attachment(const std::size_t index)
	{
		assert(index < _attachments.size());
		return _attachments[index];
	}

	const texture2d& framebuffer::get_attachment(const std::size_t index) const
	{
		assert(index < _attachments.size());
		return _attachments[index];
	}


#pragma endregion

}