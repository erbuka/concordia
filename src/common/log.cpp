#include "log.h"

#include "common.h"

#include <iostream>
#include <format>

namespace cnc::log
{
	static constexpr static_map<level, const char*, 3> s_message_levels = {
		std::pair{ level::info,		"I" },
		std::pair{ level::warning,	"W" },
		std::pair{ level::error,	"E" }
	};


	void send_message(const std::string_view message, level lvl)
	{
		std::cout << std::format("[{}] {}", s_message_levels[lvl], message) << std::endl;
	}
}