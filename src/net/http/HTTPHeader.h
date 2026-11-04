#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace zormid::net::http {
	struct HTTPHeader {
		std::unordered_map<std::string, std::string> headers;
	};

	struct HTTPRequestHeader : HTTPHeader {
		std::string type;
		std::string path;
		std::string protocol;
	};

	struct HTTPResponseHeader : HTTPHeader {
		uint16_t statusCode;
		std::string statusMessage;
	};
}
