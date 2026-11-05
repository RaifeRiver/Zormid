#include "HTTPHeader.h"

namespace zormid::net::http {
	void HTTPResponseHeader::write(const Socket& socket) const {
		socket.writeString(protocol + " " + std::to_string(statusCode) + " " + statusMessage + "\r\n");
		for (const auto& [key, value] : headers) {
			socket.writeString(key + ": " += value + "\r\n");
		}
		if (!body.empty()) {
			socket.writeString("\r\n" + body);
		}
	}
}
