#pragma once

#include <string>

namespace zormid::util::base64 {
	std::string encode(const void* input, size_t size);
}
