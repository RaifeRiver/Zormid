#include "HTTPServer.h"

namespace zormid::net::http {
	HTTPServer::HTTPServer(const uint16_t port) : mServerSocket(port) {}

	void HTTPServer::setClientHandler(const ClientHandler& handler) {
		mClientHandler = handler;
	}

	void HTTPServer::accept() const {
		if (mClientHandler.has_value()) {
			std::unique_ptr<Socket> client;
			while ((client = mServerSocket.accept())) {
				HTTPRequestHeader header;
				header.type = client->readString(' ');
				header.path = client->readString(' ');
				std::string protocol = client->readString('\n');
				header.protocol = protocol.substr(0, protocol.size() - 1);
				std::string line;
				while ((line = client->readString('\n')) != "\r") {
					const size_t splitIndex = line.find(':');
					std::string key = line.substr(0, splitIndex);
					const std::string value = line.substr(line.find_first_not_of(' ', splitIndex + 1), line.find_last_not_of('\r') - line.find_first_not_of(' ', splitIndex + 1) + 1);
					header.headers[key] = value;
				}
				(*mClientHandler)(header, std::move(client));
			}
		}
	}
}
