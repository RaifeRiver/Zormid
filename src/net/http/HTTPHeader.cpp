#include "HTTPHeader.h"

#include <cstring>

namespace zormid::net::http {
	void HTTPResponseHeader::write(Socket& socket) {
		std::string header = protocol + " " + std::to_string(statusCode) + " " + statusMessage + "\r\n";
		headers["Content-Length"] = std::to_string(strlen(body.c_str()));
		for (const auto& [key, value] : headers) {
			header += key + ": " += value + "\r\n";
		}
		header += "\r\n";
		if (!body.empty()) {
			header += body;
		}
		socket.writeString(header);
	}
}
