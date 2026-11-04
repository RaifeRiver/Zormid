#include "HTTPServer.h"

namespace zormid::net::http {
	HTTPServer::HTTPServer(const uint16_t port) : mServerSocket(port) {}

	void HTTPServer::setClientHandler(const ClientHandler& handler) {
		mClientHandler = handler;
	}

	void HTTPServer::accept() const {
		if (mClientHandler.has_value()) {
			std::optional<Socket> client;
			while ((client = mServerSocket.accept()).has_value()) {
				HTTPRequestHeader header;
				header.type = client->readString(' ');
				header.path = client->readString(' ');
				header.protocol = client->readString('\n').substr(0, -1);
				(*mClientHandler)(header, *client);
			}
		}
	}
}
