#pragma once

#include "chat/Message.h"
#include "net/http/WebSocket.h"
#include "user/User.h"

namespace zormid::client {
	class Client {
	public:
		explicit Client(std::unique_ptr<net::http::WebSocket> socket);

		void process();

		void writeAll() const;

		void close();

		[[nodiscard]] bool isOpen();

		[[nodiscard]] const user::manager::UserID& getUserID() const {
			return mUser->getID();
		}

		[[nodiscard]] user::User& getUser() const {
			return *mUser;
		}

		[[nodiscard]] net::http::WebSocket& getSocket() const {
			return *mSocket;
		}

	private:
		std::unique_ptr<net::http::WebSocket> mSocket;
		user::User* mUser = nullptr;

		void processPacket(net::http::PayloadReader& payload);

		void processAuthPacket(net::http::PayloadReader& payload);

		void processFriendListRequestPacket(net::http::PayloadReader& payload) const;

		void processUsernameRequestPacket(net::http::PayloadReader& payload) const;

		void processAcceptFriendRequestPacket(net::http::PayloadReader& payload) const;

		void processChannelListRequestPacket(net::http::PayloadReader& payload) const;

		void processMessagePacket(net::http::PayloadReader& payload) const;

		void processChannelContentsRequestPacket(net::http::PayloadReader& payload) const;

		static void processCreateChannelPacket(net::http::PayloadReader& payload);
	};
}
