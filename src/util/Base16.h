#pragma once

#include <string>

namespace zormid::util::base16 {
	std::string encode(const void* data, size_t size);
}
