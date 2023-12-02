#include "utility.h"

#include <ranges>
#include <algorithm>
#include <functional>

#include <memory>
#include <stdexcept>
#include <format>
#include <regex>
#include <string>


namespace ml
{

	std::string to_lower(const std::string_view str)
	{
		std::string result;
		result.resize(str.size());
		std::ranges::transform(str, result.begin(), [](const auto c) { return std::tolower(c); });
		return result;
	}

	std::size_t count_words(std::string_view str)
	{
		std::size_t count = 0;
		for (auto it = str.begin(); it != str.end();)
		{
			if (std::isalnum(*it))
			{
				++count;
				while (it != str.end() && std::isalnum(*it)) ++it;
			}
			else ++it;
		}
		return count;
	}

	std::list<std::string> split(const std::string_view str, const std::string_view delim)
	{
		std::list<std::string> result;
		std::size_t start = 0;
		std::size_t end = str.find(delim);
		while (end != std::string_view::npos)
		{
			result.emplace_back(str.substr(start, end - start));
			start = end + delim.size();
			end = str.find(delim, start);
		}
		result.emplace_back(str.substr(start, end));
		return result;
	}
}