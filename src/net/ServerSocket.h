#pragma once

#include <cstdint>
#include <optional>

#include "Socket.h"

namespace zormid::net {
	class ServerSocket {
	public:
		explicit ServerSocket(uint16_t port);

		[[nodiscard]] std::optional<Socket> accept() const;

		~ServerSocket();

	private:
		int mFD;
	};
}
