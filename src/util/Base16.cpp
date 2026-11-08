#include "Base16.h"

namespace zormid::util::base16 {
	constexpr char CHARS[] = "0123456789abcdef";

	std::string encode(const void* data, const size_t size) {
		std::string output;
		const auto charData = static_cast<const unsigned char*>(data);
		output.reserve((size + 1) >> 1);
		for (size_t i = 0; i < size; i++) {
			output.push_back(CHARS[charData[i] >> 4]);
			output.push_back(CHARS[charData[i] & 0xF]);
		}
		return output;
	}
}
