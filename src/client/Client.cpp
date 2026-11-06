#include "Client.h"

#include <iostream>

namespace zormid::client {
	Client::Client(std::unique_ptr<net::http::WebSocket> socket) : mSocket(std::move(socket)) {}

	void Client::process() const {
		std::optional<net::http::PayloadData> payload;
		while ((payload = mSocket->read()).has_value()) {
			std::cout << "Payload received" << std::endl;
			std::cout << std::string(payload->data, payload->size) << std::endl;
		}
	}

	void Client::close() {
		mSocket->close();
		mClosed = true;
	}
}
