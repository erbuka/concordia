#include "ui.h"

#include <core/application.h>
#include <core/texture.h>

#include "log.h"

namespace cnc
{
	void ui::set_hot(const control_id id, const bool condition)
	{
		if (is_hot(null_control) && is_active(null_control) && condition)
			_hot_control = id;
	}
	void ui::set_active(const control_id id, const bool condition)
	{
		if (is_hot(id) && is_active(null_control) && condition)
			_active_control = id;
	}

	void ui::reset_hot(const control_id id, const bool condition)
	{
		if (is_hot(id) && is_active(null_control) && condition)
			_hot_control = null_control;
	}

	void ui::reset_active(const control_id id, const bool condition)
	{
		if (is_active(id) && is_hot(id) && condition)
			_active_control = null_control;
	}

	void ui::begin_frame()
	{
		_prev_mouse_pos = _cur_mouse_pos;
		
		_cur_mouse_pos = vec2f{ app::get_mouse_pos() };
		_cur_mouse_pos[1] = app::get_window_size()[1] - _cur_mouse_pos[1];

		_prev_screen_mouse_pos = _cur_screen_mouse_pos;
		_cur_screen_mouse_pos = vec2f{ app::get_screen_mouse_pos() };

	}

	void ui::end_frame() {}

	bool ui::window_drag(vec2f& delta)
	{

		if (app::is_mouse_down(mouse_button::left))
		{
			if (is_hot(null_control) && is_active(null_control))
			{
				delta = _cur_screen_mouse_pos - _prev_screen_mouse_pos;
				return true;
			}
		}
		
		return false;

	}

	bool ui::button_area(const control_id id, const vec2f& pos, const vec2f& size)
	{
		const auto hovered = _cur_mouse_pos >= pos && _cur_mouse_pos <= pos + size;
		const auto mouse_pressed = app::is_mouse_pressed(mouse_button::left);
		const auto mouse_down = app::is_mouse_down(mouse_button::left);

		reset_hot(id, !hovered);
		reset_active(id, !mouse_down);

		set_hot(id, hovered);
		set_active(id, mouse_pressed);

		const auto value = is_hot(id) && is_active(id) && mouse_pressed;

		return value;
	}

	bool ui::button(const control_id id, const vec2f& pos, const ml::texture2d& tex)
	{
		const vec2f size{ tex.get_width(), tex.get_height() };
		const auto value = button_area(id, pos, size);

		app::texture(tex);
		app::quad(pos, size);
		app::no_texture();

		return value;
	}

	void ui::slider(const control_id id, const vec2f& pos, const vec2f& size, const vec2f& min_max, float& val, const vec4f& back_color)
	{

		const auto hovered = _cur_mouse_pos >= pos && _cur_mouse_pos <= pos + size;
		const auto mouse_pressed = app::is_mouse_pressed(mouse_button::left);
		const auto mouse_down = app::is_mouse_down(mouse_button::left);


		reset_hot(id, !hovered);
		reset_active(id, !mouse_down);

		set_hot(id, hovered);
		set_active(id, mouse_pressed);

		if (is_active(id)) {
			const float t = std::clamp(_cur_mouse_pos[0] - pos[0], 0.0f, size[0]) / size[0];
			val = std::lerp(min_max[0], min_max[1], t);
		}


		app::with([&] {
			app::color(back_color);
			app::quad(pos, size);
		});

		const float percent = std::clamp(val, min_max[0], min_max[1]) / (min_max[1] - min_max[0]);
		app::quad(pos, { size[0] * percent, size[1] });
	}
}