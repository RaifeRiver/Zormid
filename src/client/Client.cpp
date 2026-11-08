#include "Client.h"

#include <cstring>
#include <iostream>

#include "Zormid.h"

namespace zormid::client {
	Client::Client(std::unique_ptr<net::http::WebSocket> socket) : mSocket(std::move(socket)) {}

	void Client::sendMessage(const chat::Message& message) const {
		if (!mUser.has_value()) {
			return;
		}
		net::http::PayloadData payload = {};
		payload.type = net::http::PayloadType::BINARY;
		payload.size = message.content.size() + message.from.size() + 12;
		payload.data = static_cast<char*>(malloc(payload.size));
		constexpr uint32_t packetID = 1;
		memcpy(payload.data, &packetID, 4);
		const uint32_t messageLength = message.content.size();
		memcpy(payload.data + 4, &messageLength, 4);
		memcpy(payload.data + 8, message.content.data(), message.content.size());
		const uint32_t fromLength = message.from.size();
		memcpy(payload.data + 8 + message.content.size(), &fromLength, 4);
		memcpy(payload.data + 12 + message.content.size(), message.from.data(), message.from.size());
		mSocket->write(payload);
		free(payload.data);
	}

	void Client::process() {
		std::optional<net::http::PayloadData> payload;
		while ((payload = mSocket->read()).has_value()) {
			processPacket(*payload);
		}
	}

	void Client::close() const {
		mSocket->close();
	}

	bool Client::isOpen() const {
		return mSocket->isOpen();
	}

	void Client::processPacket(const net::http::PayloadData& payload) {
		const uint32_t id = payload.data[0] | payload.data[1] << 8 | payload.data[2] << 16 | payload.data[3] << 24;
		if (!mUser.has_value()) {
			if (id == 0x0) {
				char userIDChars[20];
				memcpy(userIDChars, payload.data + 4, 20);
				char sessionID[16];
				memcpy(sessionID, payload.data + 24, 16);
				const user::manager::UserID userID(userIDChars);
				for (user::User user = user::manager::loadUser(userID); user::Session session : user.getSessions()) {
					if (memcmp(sessionID, static_cast<const char*>(session), 16) == 0) {
						mUser = user;
						net::http::PayloadData response = {};
						response.type = net::http::PayloadType::BINARY;
						response.size = 4;
						response.data = static_cast<char*>(malloc(4));
						constexpr uint32_t responseID = 0x0;
						memcpy(response.data, &responseID, 4);
						mSocket->write(response);
						free(response.data);
						return;
					}
				}
			}
			return;
		}
		switch (id) {
			case 0x1:
				processMessagePacket(payload);
				break;
			case 0x2:
				processUsernameRequestPacket(payload);
				break;
			case 0x3:
				processAcceptFriendRequestPacket(payload);
				break;
			default:
				break;
		}
	}

	void Client::processMessagePacket(const net::http::PayloadData& payload) {
		const uint32_t length = payload.data[4] | payload.data[5] << 8 | payload.data[6] << 16 | payload.data[7] << 24;
		std::string message(length, 0);
		for (uint32_t i = 0; i < length; i++) {
			message[i] = payload.data[i + 8];
		}
		for (Client& c : Zormid::getInstance().getClients()) {
			c.sendMessage({.content = message, .from = mUser->getUsername()});
		}
	}

	void Client::processUsernameRequestPacket(const net::http::PayloadData& payload) const {
		char userIDChars[20];
		memcpy(userIDChars, payload.data + 4, 20);
		const user::manager::UserID userID(userIDChars);
		user::User user = user::manager::loadUser(userID);
		const std::string username = user.getUsername();
		net::http::PayloadData response = {};
		response.type = net::http::PayloadType::BINARY;
		response.size = 28 + username.length();
		response.data = static_cast<char*>(malloc(response.size));
		constexpr uint32_t responseID = 0x2;
		memcpy(response.data, &responseID, 4);
		memcpy(response.data + 4, userIDChars, 20);
		const uint32_t usernameLength = username.length();
		memcpy(response.data + 24, &usernameLength, 4);
		memcpy(response.data + 28, username.c_str(), username.length());
		mSocket->write(response);
		free(response.data);
	}

	void Client::processAcceptFriendRequestPacket(const net::http::PayloadData& payload) {
		char userIDChars[20];
		memcpy(userIDChars, payload.data + 4, 20);
		const user::manager::UserID userID(userIDChars);
		user::User user = user::manager::loadUser(userID);
		user.getFriends().push_back(mUser->getID());
		user::manager::saveUser(user);
		mUser->getFriends().push_back(userID);
		user::manager::saveUser(*mUser);
	}
}
