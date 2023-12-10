#pragma once

#include <string_view>

namespace cnc::log
{
	enum class level
	{
		info = 0,
		warning = 1,
		error = 2
	};

	void send_message(const std::string_view message, level lvl);
	inline void info(const std::string_view message) { send_message(message, level::info); }
	inline void warning(const std::string_view message) { send_message(message, level::warning); }
	inline void error(const std::string_view message) { send_message(message, level::error); }
}

#ifdef DEBUG
#define CNC_INFO(...) cnc::log::info(__VA_ARGS__)
#define CNC_ERROR(...) cnc::log::error(__VA_ARGS__)
#else
#define CNC_INFO(...)
#define CNC_ERROR(...)
#endif