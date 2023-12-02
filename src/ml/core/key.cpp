#include "key.h"

namespace ml
{
	std::string_view to_string(key k)
	{
		switch (k)
		{
		case key::none: return "none";
		case key::enter: return "enter";
		case key::backspace: return "backspace";
		case key::escape: return "escape";
		case key::left: return "left";
		case key::right: return "right";
		case key::up: return "up";
		case key::down: return "down";
		default: return "unknown";
		}
	}
}