#pragma once

#include <string>
#include <vector>

namespace zormid::util::string {
	std::string replaceAll(const std::string& str, const std::string& from, const std::string& to);

	std::vector<std::string> split(const std::string& str, const std::string& delimiter, size_t max = 0);
}
