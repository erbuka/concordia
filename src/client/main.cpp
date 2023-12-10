#include <filesystem>
#include <fstream>
#include <algorithm>
#include <ranges>

#include <core/application.h>

#include "common.h"
#include "client.h"
#include "log.h"

int main() 
{
	using namespace cnc;
	namespace fs = std::filesystem;
	
	static constexpr std::string_view s_config_file{ "config" };


	std::string host = "127.0.0.1";
	u32 port = 3000;

	if (fs::is_regular_file(s_config_file))
	{
		std::ifstream is;
		is.open(s_config_file);

		std::string line;
		while (!is.eof())
		{
			std::getline(is, line);
			const auto pos = line.find('=');

			if (pos == std::string::npos)
				continue;

			std::string name{ line.begin(), line.begin() + pos };
			std::string value{ line.begin() + pos + 1, line.end() };

			if (!name.empty())
			{
				//std::ranges::transform(name, name.begin(), [](auto c) -> char { return std::tolower(c); });
				std::ranges::transform(name, name.begin(), [](auto c) -> char { return std::tolower(c); });

				if (name == "host")
					host = value;
				else if (name == "port")
					port = std::atoi(value.c_str());


			}
		}

		CNC_INFO(std::format("Host: {}\nPort: {}", host, port));

	}

	ml::app::goto_scene(std::make_shared<cnc::voice_chat_scene>(voice_chat_config{
		.host = std::move(host),
		.port = port
	}));
	return ml::app::run({ 
		.transparent = true,
		.decorated = false,
		.resizable = false
	});
}