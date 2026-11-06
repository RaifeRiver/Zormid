#include "HTTPHeader.h"

namespace zormid::net::http {
	void HTTPResponseHeader::write(Socket& socket) const {
		std::string header = protocol + " " + std::to_string(statusCode) + " " + statusMessage + "\r\n";
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
