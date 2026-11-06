#pragma once

#include <cstdint>
#include <memory>

#include "Socket.h"

namespace zormid::net {
	class ServerSocket {
	public:
		explicit ServerSocket(uint16_t port);

		[[nodiscard]] std::unique_ptr<Socket> accept() const;

		~ServerSocket();

	private:
		int mFD;
	};
}
