#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <chrono>

#include "scene.h"
#include "utility.h"
#include "vecmath.h"
#include "font.h"
#include "texture.h"
#include "key.h"
#include "mouse.h"

namespace ml
{

	class texture;
	class font;
	class scene;

	using clock_t = std::chrono::steady_clock;
	using time_point_t = std::chrono::time_point<clock_t>;


	struct time
	{
		float delta;
		float elapsed;
		float sine_wave(const float min, const float max, const float freq) const;
	};



	class shader_program
	{
	private:
		std::uint32_t _native_handle{};
	public:

		static shader_program load(const std::string_view fs_source);

		shader_program() = default;

		shader_program(const shader_program&) = delete;
		shader_program& operator=(const shader_program&) = delete;

		shader_program(shader_program&&);
		shader_program& operator=(shader_program&&);

		~shader_program();


		std::uint32_t get_native_handle() const { return _native_handle; }

		template<typename T, std::size_t N>
		void uniform(const std::string_view name,  const vec<T, N>& v);
		
		/*
		template<>
		void uniform<std::int32_t, 1>(const std::string_view name, const vec1i& v);

		template<>
		void uniform<float, 1>(const std::string_view name, const vec1f& v);
		*/

		template<typename T>
		void uniform(const std::string_view name, const T v) { uniform(name, vec<T, 1>{ v }); }



		template<typename T>
		void uniform(const std::string_view name, T const* ptr, const std::size_t count);
		
		/*
		template<>
		void uniform<ml::vec3f>(const std::string_view name, ml::vec3f const * ptr, const std::size_t count);
		*/

	};

	namespace app
	{

		struct window_props
		{
			bool transparent{ false };
			bool decorated{ true };
			bool resizable{ true };
		};

		struct character_modifier
		{
			vec2f offset{};
			float scale_factor{ 1.0f };
			std::optional<vec4f> color_override{};
		};

		using character_modifier_fn = std::function<character_modifier(const std::uint32_t)>;



		enum class primitive_type
		{
			none,
			triangles,
			lines,
			line_strip,
			line_loop,
		};


		float rand();

		template<typename T> requires std::is_arithmetic_v<T>
		T rand(const T min, const T max)
		{
			return static_cast<T>(rand() * (max - min) + min);
		}
		
		template<typename T> requires std::is_arithmetic_v<T>
		T rand(const T max)
		{
			return rand<T>(T(0), max);
		}

		const time& get_time();

		void pivot(const vec2f& p);
		void line_width(const float w);
		void global_alpha(const float a);

		void use_program(const shader_program& prg);
		void default_program();

		void reset_context();

		void sprite(const sprite2d& spr, const vec2f& pos = {0, 0}, const vec2f& size = { 1, 1 });

		void quad(const vec2f& pos = { 0, 0}, const vec2f& size = {1, 1});
		void quad(const vec2f& pos, const vec2f& size, const vec2f uv_bl, const vec2f uv_tr);


		void begin(primitive_type type);
		void end();
		
		void viewport(const vec2i& s);
		void clear(const vec4f& c = {0.0f, 0.0f, 0.0f, 1.0f});
		void flush();

		void stroke_arc(const vec2f& pos, const float radius, const float angle_from, const float angle_to, const std::uint32_t segments = 32);
		void fill_arc(const vec2f& pos, const float radius, const float angle_from, const float angle_to, const std::uint32_t segments = 32);

		void vertex(const vec2f& v);
		void vertex(const vec3f& v);
		
		void tex_coord(const vec2f& uv);
		
		void color(const vec4f& c);
		void color(const vec3f& c);
		void color(const float c);

		void translate(const vec3f& t);
		void rotate(const vec3f& axis, const float angle);
		void scale(const vec3f& s);
		void scale(const float s);

		std::int32_t texture(const texture2d& tex);
		void no_texture();

		void set_framebuffer_srgb(const bool value);

		void draw_text(const font& fnt, const std::string& text, const float scl = 1.0f, const float line_gap = 1.0f);
		void draw_text(const font& fnt, const std::string& text, const float scl, const float line_gap, const character_modifier_fn& fn);

		const mat4f& get_projection();
		const mat4f& get_inverse_projection();
		void set_projection(const mat4f& proj);

		void push();
		void pop();
		void load_identity();

		void with(std::function<void(void)>&& f);

		void set_window_size(const vec2i& size);
		vec2i get_window_size();
		
		void set_window_pos(const vec2i& pos);
		vec2i get_window_pos();
		
		vec2f get_projection_size();
		std::pair<vec2f, vec2f> get_viewport_bounds();

		// Window
		std::int32_t run(const window_props& props = {});
		void goto_scene(std::shared_ptr<scene> s);

		// Keyboard Input
		const std::string& get_input_text();
		bool is_key_pressed(const key k);
		bool is_key_down(const key k);
		bool is_key_released(const key k);

		// Mouse Input
		
		vec2i get_mouse_pos();
		vec2i get_screen_mouse_pos();

		bool is_mouse_down(const mouse_button btn);
		bool is_mouse_pressed(const mouse_button btn);
	}


}