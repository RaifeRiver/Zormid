#pragma once

#include "net/http/WebSocket.h"

namespace zormid::client {
	class Client {
	public:
		explicit Client(std::unique_ptr<net::http::WebSocket> socket);

		void process() const;

		void close() const;

		[[nodiscard]] bool isOpen() const;

	private:
		std::unique_ptr<net::http::WebSocket> mSocket;
	};
}
