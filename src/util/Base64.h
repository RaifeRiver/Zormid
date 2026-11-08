#pragma once

#include <string>
#include <vector>

namespace zormid::util::base64 {
	std::string encode(const void* input, size_t size);

	std::vector<char> decode(const std::string& input);
}
