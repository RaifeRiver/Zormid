#pragma once

#include "net/http/WebSocket.h"

namespace zormid::client {
	class Client {
	public:
		explicit Client(std::unique_ptr<net::http::WebSocket> socket);

		void process() const;

		void close();

		[[nodiscard]] bool isClosed() const {
			return mClosed;
		}

	private:
		std::unique_ptr<net::http::WebSocket> mSocket;
		bool mClosed = false;
	};
}
