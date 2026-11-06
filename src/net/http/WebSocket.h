#pragma once

#include <memory>
#include <optional>

#include "HTTPHeader.h"
#include "net/Socket.h"

namespace zormid::net::http {
	struct PayloadData {
		char* data;
		uint64_t size;
	};

	class WebSocket {
	public:
		explicit WebSocket(std::unique_ptr<Socket> socket, HTTPRequestHeader request);

		[[nodiscard]] std::optional<PayloadData> read() const;

		[[nodiscard]] bool isOpen() const;

		void close() const;

	private:
		std::unique_ptr<Socket> mSocket;
	};
}
