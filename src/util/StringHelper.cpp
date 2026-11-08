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
}
