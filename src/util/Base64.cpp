#include "Base64.h"

#include <algorithm>
#include <iostream>

namespace zormid::util::base64 {
	constexpr char CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string encode(const void* input, const size_t size) {
		std::string output;

		int value = 0;
		int bytes = -6;
		for (size_t i = 0; i < size; i++) {
			value = (value << 8) + static_cast<const unsigned char*>(input)[i];
			bytes += 8;
			while (bytes >= 0) {
				output.push_back(CHARS[(value >> bytes) & 0x3F]);
				bytes -= 6;
			}
		}
		if (bytes > -6) {
			output.push_back(CHARS[((value << -bytes) & 0x3F)]);
		}
		while (output.size() % 4 != 0) {
			output.push_back('=');
		}
		return output;
	}

	std::vector<char> decode(const std::string& input) {
		std::vector<char> output;
		int value = 0;
		int bytes = 0;
		for (char c : input) {
			const char* charPtr = std::ranges::find(CHARS, c);
			if (charPtr == std::end(CHARS)) {
				break;
			}
			value <<= 6;
			value |= static_cast<int>(charPtr - CHARS);
			bytes += 6;
			if (bytes >= 8) {
				output.push_back(static_cast<char>(value >> (bytes - 8)));
				bytes -= 8;
			}
		}
		if (bytes > 0) {
			output.push_back(static_cast<char>(value << (8 - bytes)));
		}
		return output;
	}
}
