#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "net/Socket.h"

namespace zormid::net::http {
	struct HTTPHeader {
		std::string protocol = "HTTP/1.1";
		std::unordered_map<std::string, std::string> headers;
		std::string body;
	};

	struct HTTPRequestHeader : HTTPHeader {
		std::string type;
		std::string path;
	};

	struct HTTPResponseHeader : HTTPHeader {
		uint16_t statusCode;
		std::string statusMessage;

		void write(Socket& socket) const;
	};
}
