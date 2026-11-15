#include "Base64.h"

#include <iostream>

namespace zormid::util::base64 {
	constexpr char CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	constexpr char URL_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

	static std::string encode_(const void* input, const size_t size, const char* chars, const bool includeEquals) {
		std::string output;

		unsigned int value = 0;
		int bytes = -6;
		for (size_t i = 0; i < size; i++) {
			value = (value << 8) + static_cast<const unsigned char*>(input)[i];
			bytes += 8;
			while (bytes >= 0) {
				output.push_back(chars[(value >> bytes) & 0x3F]);
				bytes -= 6;
			}
		}
		if (bytes > -6) {
			output.push_back(chars[((value << -bytes) & 0x3F)]);
		}
		if (includeEquals) {
			while (output.size() % 4 != 0) {
				output.push_back('=');
			}
		}
		return output;
	}

	std::string encode(const void* input, const size_t size) {
		return encode_(input, size, CHARS, true);
	}

	std::string encodeURL(const void* input, const size_t size) {
		return encode_(input, size, URL_CHARS, false);
	}

	static std::vector<char> decode_(const std::string& input, const char* chars) {
		std::vector<char> output;
		unsigned int value = 0;
		int bits = 0;
		for (const char c : input) {
			int index = -1;
			for (int i = 0; i < 64; i++) {
				if (chars[i] == c) {
					index = i;
					break;
				}
			}
			if (index == -1) {
				break;
			}
			value <<= 6;
			value |= static_cast<unsigned int>(index);
			bits += 6;
			if (bits >= 8) {
				bits -= 8;
				output.push_back(static_cast<char>(value >> bits));
				value &= (1 << bits) - 1;
			}
		}
		if (bits > 0) {
			output.push_back(static_cast<char>(value << (8 - bits)));
		}
		return output;
	}

	std::vector<char> decode(const std::string& input) {
		return decode_(input, CHARS);
	}

	std::vector<char> decodeURL(const std::string& input) {
		return decode_(input, URL_CHARS);
	}
}
