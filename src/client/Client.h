#pragma once

#include "chat/Message.h"
#include "net/http/WebSocket.h"
#include "user/User.h"

namespace zormid::client {
	class Client {
	public:
		explicit Client(std::unique_ptr<net::http::WebSocket> socket);

		void sendMessage(const chat::Message& message) const;

		void process();

		void close() const;

		[[nodiscard]] bool isOpen() const;

	private:
		std::unique_ptr<net::http::WebSocket> mSocket;
		std::optional<user::User> mUser;

		void processPacket(const net::http::PayloadData& payload);

		void processMessagePacket(const net::http::PayloadData& payload);

		void processUsernameRequestPacket(const net::http::PayloadData& payload) const;

		void processAcceptFriendRequestPacket(const net::http::PayloadData& payload);
	};
}
