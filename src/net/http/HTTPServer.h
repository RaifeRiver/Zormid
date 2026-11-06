#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "HTTPHeader.h"
#include "net/ServerSocket.h"

namespace zormid::net::http {
	using ClientHandler = std::function<void(HTTPRequestHeader, std::unique_ptr<Socket>)>;

	class HTTPServer {
	public:
		explicit HTTPServer(uint16_t port);

		void setClientHandler(const ClientHandler& handler);

		void accept() const;

	private:
		ServerSocket mServerSocket;
		std::optional<ClientHandler> mClientHandler;
	};
}
