#include "ui.h"

#include <core/application.h>

#include "log.h"

namespace cnc
{
	void ui::capture_input()
	{
		_prev_mouse_pos = _cur_mouse_pos;
		
		_cur_mouse_pos = vec2f{ app::get_mouse_pos() };
		_cur_mouse_pos[1] = app::get_window_size()[1] - _cur_mouse_pos[1];

		_prev_screen_mouse_pos = _cur_screen_mouse_pos;
		_cur_screen_mouse_pos = vec2f{ app::get_screen_mouse_pos() };

	}

	bool ui::window_drag(vec2f& delta)
	{
		if (_hot_control != null_control)
			return false;

		if (app::is_mouse_down(mouse_button::left))
		{
			delta = _cur_screen_mouse_pos - _prev_screen_mouse_pos;
			return true;
		}
		else return false;

	}

	bool ui::round_button(const control_id id, const vec2f& pos, const float radius)
	{
		const auto pressed = ((_cur_mouse_pos - pos).length() <= radius && app::is_mouse_pressed(mouse_button::left));
		app::push();
		app::fill_arc(pos, radius, 0, radians(360));
		app::pop();
		return pressed;
	}
}