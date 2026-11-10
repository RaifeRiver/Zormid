#pragma once

#include "chat/Message.h"
#include "net/http/WebSocket.h"
#include "user/User.h"

namespace zormid::client {
	class Client {
	public:
		explicit Client(std::unique_ptr<net::http::WebSocket> socket);

		void sendMessage(const chat::Message& message, bool global = false) const;

		void process();

		void close() const;

		[[nodiscard]] bool isOpen() const;

		[[nodiscard]] const user::manager::UserID& getUserID() const {
			return mUser->getID();
		}

		[[nodiscard]] std::optional<user::User>& getUser() {
			return mUser;
		}

		[[nodiscard]] net::http::WebSocket& getSocket() const {
			return *mSocket;
		}

	private:
		std::unique_ptr<net::http::WebSocket> mSocket;
		std::optional<user::User> mUser;

		void processPacket(net::http::PayloadReader& payload);

		void processGlobalMessagePacket(net::http::PayloadReader& payload);

		void processUsernameRequestPacket(net::http::PayloadReader& payload) const;

		void processAcceptFriendRequestPacket(net::http::PayloadReader& payload);

		void processChannelListRequestPacket(net::http::PayloadReader& payload) const;

		void processMessagePacket(net::http::PayloadReader& payload);

		void processChannelContentsRequestPacket(net::http::PayloadReader& payload) const;
	};
}
