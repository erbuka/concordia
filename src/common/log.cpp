#include "log.h"

#include <iostream>
#include <format>

namespace cnc::log
{
	void send_message(const std::string_view message, level lvl)
	{
		std::cout << std::format("[{}] {}", static_cast<std::underlying_type_t<level>>(lvl), message) << std::endl;
	}
}