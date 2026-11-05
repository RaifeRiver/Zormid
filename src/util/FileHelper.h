#pragma once

#include <optional>
#include <string>

namespace zormid::util::file {
	std::optional<std::string> readFile(const std::string& path);
}
