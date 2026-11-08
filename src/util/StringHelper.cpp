#include "StringHelper.h"

namespace zormid::util::string {
	std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
		std::string result = str;
		size_t pos = 0;
		while ((pos = result.find(from, pos)) != std::string::npos) {
			result.replace(pos, from.length(), to);
			pos += to.length();
		}
		return result;
	}

	std::vector<std::string> split(const std::string& str, const std::string& delimiter, size_t max) {
		std::vector<std::string> result;
		size_t pos = 0;
		size_t nextPos = 0;
		while ((nextPos = str.find(delimiter, pos)) != std::string::npos) {
			result.push_back(str.substr(pos, nextPos - pos));
			pos = nextPos + delimiter.length();
			if (max != 0) {
				max--;
				if (max == 0) {
					break;
				}
			}
		}
		if (pos < str.length()) {
			result.push_back(str.substr(pos));
		}
		return result;
	}
}
