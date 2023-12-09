#pragma once

#include <unordered_map>

#include <core/vecmath.h>

#include "common.h"

namespace ml
{
	class texture2d;
}

namespace cnc
{
	using namespace ml;

	enum class ui_control_type
	{
		slider, 
		button
	};

	using control_id = u32;

	constexpr auto null_control = static_cast<control_id>(-1);


	class ui
	{
	private:

		control_id _hot_control{ null_control };
		control_id _active_control{ null_control };

		vec2f _cur_screen_mouse_pos, _prev_screen_mouse_pos;
		vec2f _cur_mouse_pos, _prev_mouse_pos;
	public:


		bool is_active(const control_id id) const { return _active_control == id; }
		bool is_hot(const control_id id) const { return _hot_control == id; }

		void set_hot(const control_id id, const bool condition);
		void set_active(const control_id id, const bool condition);

		void reset_hot(const control_id id, const bool condition);
		void reset_active(const control_id id, const bool condition);

		void begin_frame();
		void end_frame();

		bool window_drag(vec2f& delta);
		bool button_area(const control_id id, const vec2f& pos, const vec2f& size);
		bool button(const control_id id, const vec2f& pos, const texture2d& tex);
		void slider(const control_id id, const vec2f& pos, const vec2f& size, const vec2f& min_max, float& val, const vec4f& back_color = { 0.5f, 0.5f, 0.5f, 1.0f });
	};
}