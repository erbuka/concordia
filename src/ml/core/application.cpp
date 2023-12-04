#include "application.h"

#include <numeric>
#include <format>
#include <array>
#include <cassert>
#include <iostream>
#include <stack>
#include <chrono>
#include <unordered_set>
#include <random>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "texture.h"
#include "utility.h"
#include "font.h"
#include "log.h"

#ifdef _WIN32
#include <windows.h>
#endif


namespace ml
{

	float time::sine_wave(const float min, const float max, const float freq) const
	{
		return min + (max - min) * (std::sin(elapsed * freq) * 0.5f + 0.5f);
	}

}


#pragma region shader_program

namespace ml
{

	static constexpr std::string_view s_vertex_shader_source = R"glsl(
		#version 450 core

		layout(location = 0) in vec3 aPosition;
		layout(location = 1) in vec2 aUv;
		layout(location = 2) in vec4 aColor;
		layout(location = 3) in uint aTextureUnit;
		layout(location = 4) in uint aDistanceField;
		layout(location = 5) in float aDistanceFieldStep;
		uniform mat4 uProjection;

		smooth out vec2 vUv;
		smooth out vec4 vColor;
		flat out uint vTextureUnit;
		flat out uint vDistanceField;
		flat out float vDistanceFieldStep;

		void main()
		{
			gl_Position = uProjection * vec4(aPosition, 1.0);
			vUv = aUv;
			vColor = aColor;
			vTextureUnit = aTextureUnit;
			vDistanceField = aDistanceField;
			vDistanceFieldStep = aDistanceFieldStep;
		}
	)glsl";

	static constexpr std::string_view s_fragment_shader_source = R"glsl(
		
		#version 450 core

		smooth in vec2 vUv;
		smooth in vec4 vColor;
		flat in uint vTextureUnit;
		flat in uint vDistanceField;
		flat in float vDistanceFieldStep;
		
		out vec4 fOut;

		uniform sampler2D uTextures[32];

		void main()
		{
			if (vDistanceField == 0)
			{
				fOut = vColor * texture(uTextures[vTextureUnit], vUv);
			}
			else
			{
				float d = texture(uTextures[vTextureUnit], vUv).r;
				fOut = vec4(vColor.rgb, vColor.a * smoothstep(0.5 - vDistanceFieldStep, 0.5 + vDistanceFieldStep, d));
			}
		}

	)glsl";

	static std::uint32_t load_shader(const std::string_view source, GLenum shader_type)
	{
		auto shader = glCreateShader(shader_type);

		const char *src = source.data();
		const auto len = static_cast<std::int32_t>(source.size());
		glShaderSource(shader, 1, &src, &len);
		glCompileShader(shader);

		std::int32_t compile_status;

		glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

		if (!(compile_status == GL_TRUE))
		{
			TA_ERROR("Shader not compiled!");

			std::int32_t maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::string infoLog;
			infoLog.resize(maxLength, ' ');
			glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());
			TA_ERROR("InfoLog: {}", infoLog);
		}

		return shader;
	}

	shader_program::shader_program(shader_program &&other) { std::swap(_native_handle, other._native_handle); }
	shader_program &shader_program::operator=(shader_program &&other)
	{
		std::swap(_native_handle, other._native_handle);
		return *this;
	}

	shader_program::~shader_program()
	{
		if (_native_handle)
			glDeleteProgram(_native_handle);
	}

	shader_program shader_program::load(const std::string_view fs_source)
	{
		auto vertex_shader = load_shader(s_vertex_shader_source, GL_VERTEX_SHADER);
		auto fragment_shader = load_shader(fs_source, GL_FRAGMENT_SHADER);
		auto program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, fragment_shader);
		glLinkProgram(program);

		std::int32_t link_status;

		glGetProgramiv(program, GL_LINK_STATUS, &link_status);

		if (!(link_status == GL_TRUE))
		{
			TA_ERROR("Program not linked!");
			// TODO: Check program log
		}

		shader_program ret;
		ret._native_handle = program;
		return ret;
	}

	template <>
	void shader_program::uniform<std::int32_t, 1>(const std::string_view name, const vec1i &v)
	{
		glUniform1i(glGetUniformLocation(_native_handle, name.data()), v[0]);
	}

	template <>
	void shader_program::uniform<float, 1>(const std::string_view name, const vec1f &v)
	{
		glUniform1f(glGetUniformLocation(_native_handle, name.data()), v[0]);
	}

	template <>
	void shader_program::uniform<double, 1>(const std::string_view name, const vec1d& v)
	{
		glUniform1d(glGetUniformLocation(_native_handle, name.data()), v[0]);
	}

	template<>
	void shader_program::uniform<float, 2>(const std::string_view name, const vec2f& v)
	{
		glUniform2f(glGetUniformLocation(_native_handle, name.data()), v[0], v[1]);
	}

	template<>
	void shader_program::uniform<double, 2>(const std::string_view name, const vec2d& v)
	{
		glUniform2d(glGetUniformLocation(_native_handle, name.data()), v[0], v[1]);
	}


	template<>
	void shader_program::uniform<ml::vec3f>(const std::string_view name, ml::vec3f const* ptr, const std::size_t count)
	{
		glUniform3fv(glGetUniformLocation(_native_handle, name.data()), count, ptr->data.data());
	}


}

#pragma region shader

#pragma region application

namespace ml::app
{
	struct vertex3f
	{
		vec3f position{};
		vec2f uv{};
		vec4f color{};
		std::int32_t texture_unit{};
		std::int32_t distance_field{};
		float distance_field_step{};
	};

	static constexpr static_map s_key_mappings = {
			std::pair{GLFW_KEY_ENTER, key::enter},
			std::pair{GLFW_KEY_BACKSPACE, key::backspace},
			std::pair{GLFW_KEY_ESCAPE, key::escape},
			std::pair{GLFW_KEY_LEFT, key::left},
			std::pair{GLFW_KEY_RIGHT, key::right},
			std::pair{GLFW_KEY_UP, key::up},
			std::pair{GLFW_KEY_DOWN, key::down},
			std::pair{GLFW_KEY_0, key::zero},
			std::pair{GLFW_KEY_1, key::one},
			std::pair{GLFW_KEY_2, key::two},
			std::pair{GLFW_KEY_3, key::three},
			std::pair{GLFW_KEY_4, key::four},
			std::pair{GLFW_KEY_5, key::five},
			std::pair{GLFW_KEY_6, key::six},
			std::pair{GLFW_KEY_7, key::seven},
			std::pair{GLFW_KEY_8, key::eight},
			std::pair{GLFW_KEY_9, key::nine},
			std::pair{GLFW_KEY_A, key::a},
			std::pair{GLFW_KEY_B, key::b},
			std::pair{GLFW_KEY_C, key::c},
			std::pair{GLFW_KEY_D, key::d},
			std::pair{GLFW_KEY_E, key::e},
			std::pair{GLFW_KEY_F, key::f},
			std::pair{GLFW_KEY_G, key::g},
			std::pair{GLFW_KEY_H, key::h},
			std::pair{GLFW_KEY_I, key::i},
			std::pair{GLFW_KEY_J, key::j},
			std::pair{GLFW_KEY_K, key::k},
			std::pair{GLFW_KEY_L, key::l},
			std::pair{GLFW_KEY_M, key::m},
			std::pair{GLFW_KEY_N, key::n},
			std::pair{GLFW_KEY_O, key::o},
			std::pair{GLFW_KEY_P, key::p},
			std::pair{GLFW_KEY_Q, key::q},
			std::pair{GLFW_KEY_R, key::r},
			std::pair{GLFW_KEY_S, key::s},
			std::pair{GLFW_KEY_T, key::t},
			std::pair{GLFW_KEY_U, key::u},
			std::pair{GLFW_KEY_V, key::v},
			std::pair{GLFW_KEY_W, key::w},
			std::pair{GLFW_KEY_X, key::x},
			std::pair{GLFW_KEY_Y, key::y},
			std::pair{GLFW_KEY_Z, key::z}};

	static constexpr static_map s_mouse_button_mappings = {
		std::pair{ GLFW_MOUSE_BUTTON_LEFT, mouse_button::left },
		std::pair{ GLFW_MOUSE_BUTTON_RIGHT, mouse_button::right },
	};

	static expected<> initialize();
	static void terminate();
	static void vertex_internal(const vec3f &v);
	static void reset_input();

	static void glfw_character_callback(GLFWwindow *window, unsigned int codepoint);
	static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void glfw_button_callback(GLFWwindow* window, int button, int action, int mods);

	static void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

	enum class current_drawing_mode : std::uint8_t
	{
		normal = 0,
		distance_field = 1,
	};

	static struct app_context
	{
		window_props props;
		GLFWwindow *window = nullptr;
		std::shared_ptr<scene> current_scene = std::make_shared<scene>();
		std::shared_ptr<scene> next_scene = nullptr;
		std::mt19937 prng;
		std::uniform_real_distribution<float> dist01{0.0f, 1.0f};
		time current_time;
	} s_app_context;

	static struct rendering_context
	{

		struct stack_content
		{
			mat4f transform{identity<mat4f>()};
			mat4f inv_transform{identity<mat4f>()};
			vec2f pivot{0.5f, 0.5f};
			vec2f uv{0, 0};
			vec4f color{1, 1, 1, 1};
			float global_alpha{1.0f};
		};

		static constexpr std::size_t s_vertex_buffer_size = 25000 * 3;
		static constexpr std::size_t s_max_texture_units = 32;
		static constexpr auto s_texture_units_indices = []()
		{
			std::array<std::int32_t, s_max_texture_units> units{};
			for (const auto i : std::views::iota(0u, s_max_texture_units))
				units[i] = static_cast<std::int32_t>(i);
			return units;
		}();

		std::array<vertex3f, s_vertex_buffer_size> vertex_buffer{};
		std::size_t current_vertex_idx{0};

		mat4f projection{};
		mat4f inv_projection{};

		std::stack<stack_content> current{{stack_content{}}};

		current_drawing_mode current_drawing_mode{current_drawing_mode::normal};
		float current_distance_field_step{};

		primitive_type mode{};

		float line_width{1.0f};

		std::vector<vertex3f> lines_buffer{};

		texture2d white_texture{};
		uint32_t current_texture_unit_idx{};
		std::array<std::uint32_t, s_max_texture_units> texture_units{};

		// OpenGL specific
		std::uint32_t vao_id{}, vb_id{};
		shader_program default_shader{};

		void reset()
		{
			current = std::stack<stack_content>{{stack_content{}}};
			projection = identity<mat4f>();

			current_vertex_idx = 0;
			current_drawing_mode = current_drawing_mode::normal;
			current_distance_field_step = 0.0f;

			mode = primitive_type::none;

			lines_buffer = {};
			line_width = 1.0f;

			std::ranges::fill(texture_units, 0);
			texture_units[0] = white_texture.get_native_handle();
			current_texture_unit_idx = 0;
		}

	} s_rendering_context;

	static struct input_context
	{
		// Keyboard
		std::unordered_set<key> pressed_keys;
		std::unordered_set<key> released_keys;
		std::unordered_set<key> down_keys;

		std::string text;

		// Mouse
		std::unordered_set<mouse_button> down_mouse_buttons;
		std::unordered_set<mouse_button> pressed_mouse_buttons;

	} s_input_context;

	static void glfw_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		if (!s_mouse_button_mappings.contains(button))
			return;

		const auto btn_mapping = s_mouse_button_mappings[button];

		if (action == GLFW_PRESS)
		{
			s_input_context.pressed_mouse_buttons.insert(btn_mapping);
			s_input_context.down_mouse_buttons.insert(btn_mapping);
		}
		else if (action == GLFW_RELEASE)
		{
			s_input_context.down_mouse_buttons.erase(btn_mapping);
		}
	}

	static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (s_key_mappings.contains(key))
		{
			if (action == GLFW_PRESS || action == GLFW_REPEAT)
			{
				s_input_context.pressed_keys.insert(s_key_mappings[key]);
				s_input_context.down_keys.insert(s_key_mappings[key]);
			}
			else if (action == GLFW_RELEASE)
			{
				s_input_context.released_keys.insert(s_key_mappings[key]);
				s_input_context.down_keys.erase(s_key_mappings[key]);
			}
		}
	}

	static void glfw_character_callback(GLFWwindow *window, unsigned int codepoint)
	{
		s_input_context.text += static_cast<char>(codepoint);
	}

	static void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
	{
		std::cout << std::format("GL CALLBACK: type = 0x{:x}, severity = 0x{:x}, message = {}\n",
														 type, severity, message);
	}

	static void reset_input()
	{
		s_input_context.pressed_mouse_buttons.clear();
		s_input_context.pressed_keys.clear();
		s_input_context.released_keys.clear();
		s_input_context.text.clear();
	}

	static expected<> initialize()
	{
		/* Initialize GLFW */
		if (!glfwInit())
			return unexpected{};

#ifdef DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

		if (s_app_context.props.transparent)
			glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);

		glfwWindowHint(GLFW_DECORATED, s_app_context.props.decorated);
		glfwWindowHint(GLFW_RESIZABLE, s_app_context.props.resizable);
			

		/* Create a windowed mode window and its OpenGL context */
		s_app_context.window = glfwCreateWindow(1280, 768, "Hello World", NULL, NULL);
		if (!s_app_context.window)
			return unexpected{};

		/* Make the window's context current */
		glfwMakeContextCurrent(s_app_context.window);
		glfwSetCharCallback(s_app_context.window, glfw_character_callback);
		glfwSetKeyCallback(s_app_context.window, glfw_key_callback);
		glfwSetMouseButtonCallback(s_app_context.window, glfw_button_callback);

		gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

		auto &ctx = s_rendering_context;

		// Initialize the white texture, and bind it to texture unit 0
		const auto white_pixel = 0xffffffff;
		ctx.white_texture = texture2d::load(texture_format::rgba8, 1, 1, texture_filter_mode::nearest, &white_pixel);
		ctx.texture_units[0] = ctx.white_texture.get_native_handle();

		// Intialize OpenGL

#ifdef DEBUG
		glDebugMessageCallback(gl_debug_callback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


		// Load Shaders
		ctx.default_shader = shader_program::load(s_fragment_shader_source);

		// Initialize the batch renderer vertex array object
		glCreateBuffers(1, &ctx.vb_id);
		//glBindBuffer(GL_ARRAY_BUFFER, ctx.vb_id);
		glNamedBufferData(ctx.vb_id, sizeof(rendering_context::vertex_buffer), nullptr, GL_DYNAMIC_DRAW);

		glCreateVertexArrays(1, &ctx.vao_id);
		glVertexArrayVertexBuffer(ctx.vao_id, 0, ctx.vb_id, 0, sizeof(vertex3f));

		glEnableVertexArrayAttrib(ctx.vao_id, 0);
		glEnableVertexArrayAttrib(ctx.vao_id, 1);
		glEnableVertexArrayAttrib(ctx.vao_id, 2);
		glEnableVertexArrayAttrib(ctx.vao_id, 3);
		glEnableVertexArrayAttrib(ctx.vao_id, 4);
		glEnableVertexArrayAttrib(ctx.vao_id, 5);

		glVertexArrayAttribFormat(ctx.vao_id, 0, 3, GL_FLOAT, GL_FALSE, offsetof(vertex3f, position));
		glVertexArrayAttribFormat(ctx.vao_id, 1, 2, GL_FLOAT, GL_FALSE, offsetof(vertex3f, uv));
		glVertexArrayAttribFormat(ctx.vao_id, 2, 4, GL_FLOAT, GL_FALSE, offsetof(vertex3f, color));
		glVertexArrayAttribIFormat(ctx.vao_id, 3, 1, GL_INT, offsetof(vertex3f, texture_unit));
		glVertexArrayAttribIFormat(ctx.vao_id, 4, 1, GL_INT, offsetof(vertex3f, distance_field));
		glVertexArrayAttribFormat(ctx.vao_id, 5, 4, GL_FLOAT, GL_FALSE, offsetof(vertex3f, distance_field_step));

		glVertexArrayAttribBinding(ctx.vao_id, 0, 0);
		glVertexArrayAttribBinding(ctx.vao_id, 1, 0);
		glVertexArrayAttribBinding(ctx.vao_id, 2, 0);
		glVertexArrayAttribBinding(ctx.vao_id, 3, 0);
		glVertexArrayAttribBinding(ctx.vao_id, 4, 0);
		glVertexArrayAttribBinding(ctx.vao_id, 5, 0);

		return default_success{};
	}

	static void terminate()
	{
		auto &app_ctx = s_app_context;

		if (app_ctx.current_scene)
		{
			app_ctx.current_scene->on_detach();
			app_ctx.current_scene = nullptr;
		}

		glfwTerminate();
	}

	void clear(const vec4f &c)
	{
		glClearColor(c[0], c[1], c[2], c[3]);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void flush()
	{

		auto &ctx = s_rendering_context;

		std::int32_t program;
		glGetIntegerv(GL_CURRENT_PROGRAM, &program);

		if (ctx.current_vertex_idx > 0)
		{

			glNamedBufferSubData(ctx.vb_id, 0, sizeof(vertex3f) * ctx.current_vertex_idx, ctx.vertex_buffer.data());

			glUniformMatrix4fv(glGetUniformLocation(program, "uProjection"), 1, GL_TRUE, ctx.projection.ptr());

			glBindTextures(0, rendering_context::s_max_texture_units, ctx.texture_units.data());

			if (program == ctx.default_shader.get_native_handle())
				glUniform1iv(glGetUniformLocation(program, "uTextures"), rendering_context::s_max_texture_units, rendering_context::s_texture_units_indices.data());

			glBindVertexArray(ctx.vao_id);
			glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(ctx.current_vertex_idx));
			glBindVertexArray(0);
		}

		ctx.current_vertex_idx = 0;
		ctx.current_texture_unit_idx = 0;
		ctx.texture_units.fill(0);
		ctx.texture_units[0] = ctx.white_texture.get_native_handle();
	}

	static void vertex_internal(const vec3f &v)
	{
		auto &ctx = s_rendering_context;

		ctx.vertex_buffer[ctx.current_vertex_idx++] = vertex3f{
				.position = vec3f{ctx.current.top().transform *vec4f{v, 1.0f}},
				.uv = ctx.current.top().uv,
				.color = ctx.current.top().color *vec4f{1.0f, 1.0f, 1.0f, ctx.current.top().global_alpha},
				.texture_unit = static_cast<std::int32_t>(ctx.current_texture_unit_idx),
				.distance_field = static_cast<std::int32_t>(ctx.current_drawing_mode),
				.distance_field_step = ctx.current_distance_field_step};

		if (ctx.current_vertex_idx == ctx.s_vertex_buffer_size)
		{
			flush();
		}
	}

	void line_width(const float width) { s_rendering_context.line_width = width; }

	void reset_context()
	{
		s_rendering_context.reset();
		default_program();
	}

	void use_program(const shader_program &prg) { glUseProgram(prg.get_native_handle()); }
	void default_program() { glUseProgram(s_rendering_context.default_shader.get_native_handle()); }

	void sprite(const sprite2d &spr, const vec2f &pos, const vec2f &size)
	{
		texture(spr.tex);
		quad(pos, size, spr.uv_bottom_left, spr.uv_top_right);
		no_texture();
	}

	float rand()
	{
		return s_app_context.dist01(s_app_context.prng);
	}

	void global_alpha(const float a)
	{
		s_rendering_context.current.top().global_alpha *= a;
	}

	void pivot(const vec2f &p)
	{
		assert(s_rendering_context.mode == primitive_type::none);
		s_rendering_context.current.top().pivot = p;
	}

	void fill_arc(const vec2f &pos, const float radius, const float angle_from, const float angle_to, const std::uint32_t segments)
	{
		const auto &ctx = s_rendering_context;
		assert(ctx.mode == primitive_type::none);

		const auto step = (angle_to - angle_from) / segments;
		begin(primitive_type::triangles);

		for (const auto i : std::views::iota(0u, segments))
		{
			const auto angle = angle_from + step * i;
			vertex(pos);
			vertex(pos + vec2f{std::cos(angle), std::sin(angle)} * radius);
			vertex(pos + vec2f{std::cos(angle + step), std::sin(angle + step)} * radius);
		}
		end();
	}

	void stroke_arc(const vec2f &pos, const float radius, const float angle_from, const float angle_to, const std::uint32_t segments)
	{
		const auto &ctx = s_rendering_context;
		assert(ctx.mode == primitive_type::none);

		const auto step = (angle_to - angle_from) / segments;

		begin(primitive_type::line_strip);

		for (const auto i : std::views::iota(0u, segments + 1))
		{
			const auto angle = angle_from + step * i;
			vertex(pos + vec2f{std::cos(angle), std::sin(angle)} * radius);
		}

		end();
	}

	void quad(const vec2f &pos, const vec2f &size)
	{
		quad(pos, size, {0, 0}, {1, 1});
	}

	void quad(const vec2f &pos, const vec2f &size, const vec2f uv_bl, const vec2f uv_tr)
	{
		const auto &ctx = s_rendering_context;
		assert(ctx.mode == primitive_type::none);

		const auto &pivot = ctx.current.top().pivot;

		const std::array<vec2f, 4> vertices = {
				vec2f{pos[0] - pivot[0] * size[0], pos[1] - pivot[1] * size[1]},
				vec2f{pos[0] + size[0] - pivot[0] * size[0], pos[1] - pivot[1] * size[1]},
				vec2f{pos[0] + size[0] - pivot[0] * size[0], pos[1] + size[1] - pivot[1] * size[1]},
				vec2f{pos[0] - pivot[0] * size[0], pos[1] + size[1] - pivot[1] * size[1]},
		};

		const std::array<vec2f, 4> uvs = {
				vec2f{uv_bl[0], uv_bl[1]},
				vec2f{uv_tr[0], uv_bl[1]},
				vec2f{uv_tr[0], uv_tr[1]},
				vec2f{uv_bl[0], uv_tr[1]}};

		for (const auto i : {0u, 1u, 2u})
		{
			tex_coord(uvs[i]);
			vertex_internal(vec3f{vertices[i], 0.0f});
		}

		for (const auto i : {0u, 2u, 3u})
		{
			tex_coord(uvs[i]);
			vertex_internal(vec3f{vertices[i], 0.0f});
		}
	}

	void begin(primitive_type type)
	{
		assert(s_rendering_context.mode == primitive_type::none);
		s_rendering_context.mode = type;
	}

	void end()
	{
		auto &ctx = s_rendering_context;
		assert(ctx.mode != primitive_type::none);

		static constexpr auto perp2 = []<typename T>(const T &start, const T &end)
		{
			return vec2f(end - start).normalized().perp();
		};

		static constexpr auto perp3 = []<typename T>(const T &start, const T &p, const T &end)
		{
			const auto n0 = vec2f(p - start).normalized().perp();
			const auto n1 = vec2f(end - p).normalized().perp();
			return (n0 + n1).normalized();
		};

		static constexpr auto line = [](const vertex3f &a, const vertex3f &b, const vec2f &na, const vec2f &nb)
		{
			const auto &ctx = s_rendering_context;
			const auto dir0 = vec2f(b.position - a.position).normalized();
			const auto dir1 = vec2f(a.position - b.position).normalized();
			const auto [cos_0a, cos1a] = std::pair{dot(dir0, na), dot(dir1, na)};
			const auto [cos_0b, cos1b] = std::pair{dot(dir0, nb), dot(dir1, nb)};
			const float cos_a = cos_0a > 0 ? cos_0a : cos1a;
			const float cos_b = cos_0b > 0 ? cos_0b : cos1b;
			const float len_a = ctx.line_width / std::sqrt(1.0f - cos_a * cos_a);
			const float len_b = ctx.line_width / std::sqrt(1.0f - cos_b * cos_b);

			color(a.color);
			tex_coord({0, 1});
			vertex_internal(a.position + vec3f{na, 0.0f} * len_a);
			tex_coord({0, 0});
			vertex_internal(a.position - vec3f{na, 0.0f} * len_a);
			color(b.color);
			tex_coord({1, 0});
			vertex_internal(b.position - vec3f{nb, 0.0f} * len_b);

			tex_coord({1, 0});
			vertex_internal(b.position - vec3f{nb, 0.0f} * len_b);
			tex_coord({1, 1});
			vertex_internal(b.position + vec3f{nb, 0.0f} * len_b);
			color(a.color);
			tex_coord({0, 1});
			vertex_internal(a.position + vec3f{na, 0.0f} * len_a);
		};

		static constexpr auto update_lines_with_pivot = []
		{
			/*
			auto& ctx = s_rendering_context;

			vec2f min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
			vec2f max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

			for (const auto& v : ctx.lines_buffer)
				for (auto i : { 0u, 1u })
				{
					min[i] = std::min(min[i], v.position[i]);
					max[i] = std::max(max[i], v.position[i]);
				}

			const vec2f correction = (max - min) * -ctx.current.top().pivot;

			for(auto& v: ctx.lines_buffer)
				v.position = v.position + vec3f{ correction, 0.0f };

			*/
		};

		if (ctx.mode == primitive_type::lines)
		{
			assert(ctx.lines_buffer.size() % 2 == 0);

			// TODO: implement pivot (take the bounding box of all vertices as the reference)
			update_lines_with_pivot();

			for (std::size_t i = 0; i < ctx.lines_buffer.size(); i += 2)
			{
				const auto &a = ctx.lines_buffer[i];
				const auto &b = ctx.lines_buffer[i + 1];
				const auto n = perp2(a.position, b.position);
				line(a, b, n, n);
			}
		}
		else if (ctx.mode == primitive_type::line_strip || ctx.mode == primitive_type::line_loop)
		{

			assert(ctx.lines_buffer.size() >= 3);

			// TODO: implement pivot (take the bounding box of all vertices as the reference)
			update_lines_with_pivot();

			const auto lc = ctx.lines_buffer.size();

			std::vector<vec2f> perps(ctx.lines_buffer.size());

			for (std::size_t i = 1; i < lc - 1; ++i)
				perps[i] = perp3(ctx.lines_buffer[i - 1].position, ctx.lines_buffer[i].position, ctx.lines_buffer[i + 1].position);

			if (ctx.mode == primitive_type::line_strip)
			{
				perps[0] = perp2(ctx.lines_buffer[0].position, ctx.lines_buffer[1].position);
				perps[lc - 1] = perp2(ctx.lines_buffer[lc - 2].position, ctx.lines_buffer[lc - 1].position);
			}
			else
			{
				perps[0] = perp3(ctx.lines_buffer[lc - 1].position, ctx.lines_buffer[0].position, ctx.lines_buffer[1].position);
				perps[lc - 1] = perp3(ctx.lines_buffer[lc - 2].position, ctx.lines_buffer[lc - 1].position, ctx.lines_buffer[0].position);
			}

			for (std::size_t i = 0; i < lc - 1; ++i)
				line(ctx.lines_buffer[i], ctx.lines_buffer[i + 1], perps[i], perps[i + 1]);

			if (ctx.mode == primitive_type::line_loop)
				line(ctx.lines_buffer[lc - 1], ctx.lines_buffer[0], perps[lc - 1], perps[0]);
		}

		ctx.lines_buffer.clear();
		ctx.mode = primitive_type::none;
	}

	void vertex(const vec2f &v) { vertex(vec3f{v[0], v[1], 0.0f}); }

	void vertex(const vec3f &v)
	{
		assert(s_rendering_context.mode != primitive_type::none);

		if (s_rendering_context.mode == primitive_type::triangles)
		{
			vertex_internal(v);
		}
		else if (s_rendering_context.mode == primitive_type::lines ||
						 s_rendering_context.mode == primitive_type::line_strip ||
						 s_rendering_context.mode == primitive_type::line_loop)
		{
			s_rendering_context.lines_buffer.push_back({
					.position = v,
					.color = s_rendering_context.current.top().color,
			});
		}
	}

	void tex_coord(const vec2f &uv) { s_rendering_context.current.top().uv = uv; }

	void color(const vec4f &c) { s_rendering_context.current.top().color = c; }
	void color(const vec3f &c) { color(vec4f{c, 1.0f}); }

	void color(const float c) { color({c, c, c, 1.0f}); }

	void translate(const vec3f &t)
	{
		auto &ctx = s_rendering_context.current.top();
		ctx.transform = ctx.transform * ml::get_translation(t);
		ctx.inv_transform = ctx.inv_transform * ml::get_translation(t * -1.0f);
	}

	void rotate(const vec3f &axis, const float angle)
	{
		auto &ctx = s_rendering_context.current.top();
		ctx.transform = ctx.transform * ml::get_rotation(axis, angle);
		ctx.inv_transform = ctx.inv_transform * ml::get_rotation(axis, -angle);
	}

	void scale(const vec3f &s)
	{
		auto &ctx = s_rendering_context.current.top();
		ctx.transform = ctx.transform * ml::get_scale(s);
		ctx.inv_transform = ctx.inv_transform * ml::get_scale(vec3f{1.0f, 1.0f, 1.0f} / s);
	}

	void scale(const float s) { scale({s, s, s}); }

	std::int32_t texture(const texture2d &tex)
	{
		auto &ctx = s_rendering_context;

		if (auto it = std::ranges::find_if(ctx.texture_units, [&](const auto tex_id)
																			 { return tex_id == tex.get_native_handle(); });
				it != ctx.texture_units.end())
		{
			// If the texture is already bound to a texture unit, we just need to calculcate the index as the distance between the begin and the iterator
			ctx.current_texture_unit_idx = static_cast<std::uint32_t>(std::distance(ctx.texture_units.begin(), it));
		}
		else
		{
			// If the texture is not bound to a texture unit, I need to find a free texture unit (which is the first texture unit with a value of 0, not counting the first which is the white texture)
			it = std::find_if(ctx.texture_units.begin() + 1, ctx.texture_units.end(),
												[](const auto tex_id)
												{ return tex_id == 0; });

			if (it == ctx.texture_units.end())
			{
				// Could not find a free texture unit. I flush, which will reset all the texture units
				// and then call this function again, which will find a free texture unit
				flush();
				texture(tex);
			}
			else
			{
				// Found a free texture unit, bind the texture to it and update the current texture unit index
				*it = tex.get_native_handle();
				ctx.current_texture_unit_idx = static_cast<std::uint32_t>(std::distance(ctx.texture_units.begin(), it));
			}
		}

		return rendering_context::s_texture_units_indices[ctx.current_texture_unit_idx];
	}

	void no_texture()
	{
		// Sets the white texture as the current texture
		s_rendering_context.current_texture_unit_idx = 0;
	}

	void set_framebuffer_srgb(const bool value)
	{
		value ? glEnable(GL_FRAMEBUFFER_SRGB) : glDisable(GL_FRAMEBUFFER_SRGB);
	}

	void draw_text(const font &fnt, const std::string &text, const float scl, const float line_gap)
	{
		draw_text(fnt, text, scl, line_gap, [](const std::int32_t) -> character_modifier
							{ return {}; });
	}

	void draw_text(const font &fnt, const std::string &text, const float scl, const float line_gap, const character_modifier_fn &fn)
	{

		assert(s_rendering_context.mode == primitive_type::none);

		auto &ctx = s_rendering_context;

		const auto window_size = get_window_size();
		const auto font_size = fnt.get_ascent() + fnt.get_descent(); // going to be 1.0f all the time

		// Get the font size in pixels
		const auto screen_size =
				(vec3f(ctx.projection * ctx.current.top().transform *vec4f{0.0f, scl, 0.0f, 0.0f}) * vec3f{window_size[0] / 2, window_size[1] / 2, 0.0f}).length();

		// If max length is greater than 0, we need to trim each line to fit the max length
		// std::string prepared_text = max_length > 0.0f ? fnt.shrink_to_fit(text, max_length) : std::string{ text.begin(), text.end() };

		// Count the text lines (1 + count(line breaks))
		const std::size_t line_count = std::ranges::count(text, '\n') + 1;

		// If there are multiple lines, we put a gap between them
		const float str_height = line_count * font_size + (line_count - 1) * line_gap;

		// The width of the string is the width of the widest line
		const float str_width = fnt.get_string_width(text);

		texture(fnt.get_font_texture());

		push();
		scale({scl, scl, 1.0f});
		const auto &pivot = ctx.current.top().pivot;
		const auto starting_x = -pivot[0] * str_width;
		const auto line_spacing = font_size + line_gap;
		float current_x = starting_x;

		// We have to start from the top line and go down
		float current_y = -pivot[1] * str_height + line_spacing * (line_count - 1);

		// Set drawing mode to "distance field"
		ctx.current_drawing_mode = current_drawing_mode::distance_field;
		ctx.current_distance_field_step = font::sample_size / screen_size * 0.01f;

		for (const auto [index, c] : text | ranges::enumerate)
		{
			if (c == '\n')
			{
				current_x = starting_x;
				current_y -= line_spacing;
				continue;
			}

			const auto &cinfo = fnt.get_code_point(c);
			const auto modifier = fn(static_cast<std::uint32_t>(index));

			const vec3f pos{current_x + cinfo.offset[0] + modifier.offset[0], current_y + cinfo.offset[1] + modifier.offset[1], 0.0f};
			const auto w = cinfo.size[0] * modifier.scale_factor;
			const auto h = cinfo.size[1] * modifier.scale_factor;

			color(modifier.color_override ? modifier.color_override.value() : ctx.current.top().color);

			tex_coord({cinfo.uv_top_left[0], cinfo.uv_bottom_right[1]});
			vertex_internal(pos + vec3f{0.0f, 0.0f, 0.0f}); // bottom left
			tex_coord({cinfo.uv_bottom_right[0], cinfo.uv_bottom_right[1]});
			vertex_internal(pos + vec3f{w, 0.0f, 0.0f}); // bottom right
			tex_coord({cinfo.uv_bottom_right[0], cinfo.uv_top_left[1]});
			vertex_internal(pos + vec3f{w, h, 0.0f}); // top right

			tex_coord({cinfo.uv_top_left[0], cinfo.uv_bottom_right[1]});
			vertex_internal(pos + vec3f{0.0f, 0.0f, 0.0f}); // bottom left
			tex_coord({cinfo.uv_bottom_right[0], cinfo.uv_top_left[1]});
			vertex_internal(pos + vec3f{w, h, 0.0f}); // top right
			tex_coord({cinfo.uv_top_left[0], cinfo.uv_top_left[1]});
			vertex_internal(pos + vec3f{0.0f, h, 0.0f}); // top left

			current_x += cinfo.advance;
		}

		pop();
		no_texture();

		// Set drawing mode back to "normal"
		ctx.current_drawing_mode = current_drawing_mode::normal;
	}

	const mat4f& get_projection() { return s_rendering_context.projection; }
	const mat4f& get_inverse_projection() { return s_rendering_context.inv_projection; }

	void set_projection(const mat4f &proj)
	{
		auto &ctx = s_rendering_context;
		ctx.projection = proj;
		ctx.inv_projection = get_inverse(proj);
	}

	void push()
	{
		s_rendering_context.current.push(s_rendering_context.current.top());
	}

	void pop()
	{
		s_rendering_context.current.pop();
	}

	void load_identity()
	{
		s_rendering_context.current.top().transform = ml::identity<mat4f>();
	}

	void with(std::function<void(void)> &&f)
	{
		push();
		f();
		pop();
	}

	void set_window_size(const vec2i& size)
	{
		glfwSetWindowSize(s_app_context.window, size[0], size[1]);
	}

	vec2i get_window_size()
	{
		vec2i s;
		glfwGetWindowSize(s_app_context.window, &s[0], &s[1]);
		return s;
	}

	void set_window_pos(const vec2i& pos) { glfwSetWindowPos(s_app_context.window, pos[0], pos[1]); }

	vec2i get_window_pos()
	{
		std::int32_t x, y;
		glfwGetWindowPos(s_app_context.window, &x, &y);
		return { x, y };
	}

	vec2f get_projection_size()
	{
		const auto [bl, tr] = get_viewport_bounds();
		return {tr[0] - bl[0], tr[1] - bl[1]};
	}

	// TODO: like above, rename this to something more appropriate
	std::pair<vec2f, vec2f> get_viewport_bounds()
	{
		const auto &ctx = s_rendering_context;
		const auto bl = ctx.inv_projection *vec4f{-1.0f, -1.0f, 0.0f, 1.0f};
		const auto tr = ctx.inv_projection *vec4f{1.0f, 1.0f, 0.0f, 1.0f};
		return {{bl[0], bl[1]}, {tr[0], tr[1]}};
	}

	void viewport(const vec2i &s) { glViewport(0, 0, s[0], s[1]); }

	const time &get_time() { return s_app_context.current_time; }

	std::int32_t run(const window_props& props)
	{

		auto &app_ctx = s_app_context;
		s_app_context.props = props;

		if (initialize().is_error())
		{
			terminate();
			return -1;
		}

		using clock_t = std::chrono::high_resolution_clock;

		const auto start_time = std::chrono::high_resolution_clock::now();
		auto curr_time = clock_t::now();
		auto prev_time = clock_t::now();

		/* Loop until the user closes the window */
		while (!glfwWindowShouldClose(s_app_context.window))
		{
			auto &ctx = s_rendering_context;

			const auto ws = get_window_size();
			glViewport(0, 0, ws[0], ws[1]);

			if (app_ctx.next_scene != nullptr)
			{
				app_ctx.current_scene->on_detach();
				std::swap(app_ctx.current_scene, app_ctx.next_scene);
				app_ctx.current_scene->on_attach();
				app_ctx.next_scene = nullptr;
			}
      
			curr_time = clock_t::now();
			app_ctx.current_time = {
					.delta = std::chrono::duration<float>(curr_time - prev_time).count(),
					.elapsed = std::chrono::duration<float>(curr_time - start_time).count()};
			prev_time = curr_time;

			glfwSetWindowTitle(app_ctx.window, std::format("FPS: {:.2f}", 1.0f / app_ctx.current_time.delta).c_str());

			app::reset_context();

			app_ctx.current_scene->on_before_update();
			app_ctx.current_scene->on_update();
			app_ctx.current_scene->on_after_update();

			// Reset input state
			reset_input();

			/* Swap front and back buffers */
			glfwSwapBuffers(s_app_context.window);

			/* Poll for and process events */
			glfwPollEvents();
		}

		terminate();
		return 0;
	}

	void goto_scene(std::shared_ptr<scene> s) { s_app_context.next_scene = std::move(s); }

	const std::string& get_input_text() { return s_input_context.text; }

	bool is_key_pressed(const key k) { return s_input_context.pressed_keys.contains(k); }

	bool is_key_down(const key k) { return s_input_context.down_keys.contains(k); }

	bool is_key_released(const key k) { return s_input_context.released_keys.contains(k); }

	bool is_mouse_down(const mouse_button btn) { return s_input_context.down_mouse_buttons.contains(btn); }

	bool is_mouse_pressed(const mouse_button btn) { return s_input_context.pressed_mouse_buttons.contains(btn); }

	vec2i get_mouse_pos()
	{
		double x, y;
		glfwGetCursorPos(s_app_context.window, &x, &y);
		return { x, y };
	}

	vec2i get_screen_mouse_pos()
	{
#ifdef _WIN32
		POINT p;
		GetCursorPos(&p);
		return vec2i(static_cast<std::int32_t>(p.x), static_cast<std::int32_t>(p.y));
#else
		throw std::runtime_error("Unimplemented");
#endif
	}

}

#pragma endregion