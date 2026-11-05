#include "Base64.h"

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
			output.push_back(CHARS[((value << 8) >> (value + 8)) & 0x3F]);
		}
		while (output.size() % 4 != 0) {
			output.push_back('=');
		}
		return output;
	}
}