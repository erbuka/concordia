#pragma once

#include <iostream>
#include <format>
#include <array>

namespace ml::log
{
	enum class level : std::uint8_t
	{
		info = 0,
		warning,
		error
	};


	template<typename... Args>
	void message(const level lvl, const std::string& message)
	{
		const auto level_name = std::array<std::string_view, 3>{
			"INFO",
			"WARN",
			"ERRO"
		}[static_cast<std::underlying_type_t<level>>(lvl)] ;

		std::cout << std::format("[{}]: {}\n", level_name, message);
	}

}

#ifdef DEBUG
#define TA_INFO(...) ml::log::message(ml::log::level::info, std::format(__VA_ARGS__))
#define TA_WARNING(...) ml::log::message(ml::log::level::warning, std::format(__VA_ARGS__))
#define TA_ERROR(...) ml::log::message(ml::log::level::error, std::format(__VA_ARGS__))
#else
#define TA_INFO(...)
#define TA_WARNING(...)
#define TA_ERROR(...)
#endif // DEBUG