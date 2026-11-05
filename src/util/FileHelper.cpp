#include "FileHelper.h"

#include <fstream>
#include <sstream>

namespace zormid::util::file {
	std::optional<std::string> readFile(const std::string& path) {
		std::ifstream file(path);
		if (!file.is_open()) {
			return {};
		}
		std::stringstream data;
		data << file.rdbuf();
		return data.str();
	}
}
