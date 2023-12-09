#include <ranges>
#include <algorithm>
#include <vector>

#include "common.h"


int main() {
	using namespace cnc;

	std::vector<sample_t> samples;
		
	history_buffer buffer;

	for (auto i : std::views::iota(0, 200))
		samples.push_back(rand());

	std::ranges::copy(samples, std::back_inserter(buffer));

	getchar();
}