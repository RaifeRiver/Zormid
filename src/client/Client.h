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
		std::optional<chat::ChannelID> mTypingChannel;
		std::optional<chat::ChannelID> mCallChannel;

		void sendUserInfoToAll() const;

		void sendTypingToAllInChannels() const;

		void close_();

		void processPacket(net::http::PayloadReader& payload);

		void processAuthPacket(net::http::PayloadReader& payload);

		void processFriendListRequestPacket() const;

		void processUsernameRequestPacket(net::http::PayloadReader& payload) const;

		void processAcceptFriendRequestPacket(net::http::PayloadReader& payload) const;

		void processChannelListRequestPacket(net::http::PayloadReader& payload) const;

		void processMessagePacket(net::http::PayloadReader& payload) const;

		void processChannelInfoRequestPacket(net::http::PayloadReader& payload) const;

		void processChannelMessageRequestPacket(net::http::PayloadReader& payload) const;

		void processChangeUsernamePacket(net::http::PayloadReader& payload) const;

		void processFileUploadRequestPacket(net::http::PayloadReader& payload) const;

		void processSessionsRequestPacket() const;

		void processLogoutSessionPacket(net::http::PayloadReader& payload) const;

		void processSetAvatarPacket(net::http::PayloadReader& payload) const;

		void processTypingPacket(net::http::PayloadReader& payload);

		void processJoinCallPacket(net::http::PayloadReader& payload);

		void processCallOfferPacket(net::http::PayloadReader& payload) const;

		void processCallCandidatePacket(net::http::PayloadReader& payload) const;

		void processLeaveCallPacket();

		static void processFileUploadPacket(net::http::PayloadReader& payload) ;

		static void processChangeChannelUsersPacket(net::http::PayloadReader& payload);

		static void processCreateChannelPacket(net::http::PayloadReader& payload);
	};
}
