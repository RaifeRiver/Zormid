#include "ServerSocket.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>

namespace zormid::net {
	ServerSocket::ServerSocket(const uint16_t port) {
		mFD = socket(AF_INET, SOCK_STREAM, 0);
		if (mFD == -1) {
			throw std::runtime_error("Failed to create socket");
		}
		if (fcntl(mFD, F_SETFL, fcntl(mFD, F_GETFL) | O_NONBLOCK) == -1) {
			throw std::runtime_error("Failed to set socket flags");
		}
		constexpr int opt = 1;
		if (setsockopt(mFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
			throw std::runtime_error("Failed to set socket options");
		}
		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);
		if (bind(mFD, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
			throw std::runtime_error("Failed to bind socket");
		}
		if (listen(mFD, SOMAXCONN) == -1) {
			throw std::runtime_error("Failed to listen on socket");
		}
	}

	std::optional<Socket> ServerSocket::accept() const {
		sockaddr_in addr = {};
		socklen_t len = sizeof(addr);
		const int fd = ::accept(mFD, reinterpret_cast<sockaddr*>(&addr), &len);
		if (fd == -1) {
			return {};
		}
		if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1) {
			return {};
		}
		return Socket(fd);
	}

	ServerSocket::~ServerSocket() {
		close(mFD);
	}
}
