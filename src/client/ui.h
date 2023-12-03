#pragma once

#include <unordered_map>

#include <core/vecmath.h>

#include "common.h"


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

		void capture_input();
		bool window_drag(vec2f& delta);
		bool round_button(const control_id id, const vec2f& pos, const float radius);
	};
}