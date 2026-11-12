#include "Client.h"

#include <cstring>
#include <iostream>

#include "Zormid.h"

namespace zormid::client {
	Client::Client(std::unique_ptr<net::http::WebSocket> socket) : mSocket(std::move(socket)) {}

	void Client::process() {
		std::optional<net::http::PayloadData> payload;
		while ((payload = mSocket->read()).has_value()) {
			net::http::PayloadReader payloadReader(*payload);
			processPacket(payloadReader);
		}
	}

	void Client::writeAll() const {
		if (!mUser) {
			return;
		}
		for (net::http::PayloadData& writePayload : mUser->getWriteQueue()) {
			mSocket->write(writePayload);
		}
	}

	void Client::close() {
		mSocket->close();
		if (mUser) {
			user::manager::unloadUser(mUser->getID());
			mUser = nullptr;
		}
	}

	bool Client::isOpen() {
		const bool open = mSocket->isOpen();
		if (!open && mUser) {
			user::manager::unloadUser(mUser->getID());
			mUser = nullptr;
		}
		return open;
	}

	void Client::processPacket(net::http::PayloadReader& payload) {
		uint32_t id = 0;
		payload >> id;
		if (!mUser) {
			if (id == 0x0) {
				const user::manager::UserID userID = payload.readUserID();
				const user::Session sessionID = payload.readSession();
				for (user::User& user = user::manager::loadUser(userID); user::Session session : user.getSessions()) {
					if (memcmp(static_cast<const char*>(sessionID), static_cast<const char*>(session), 16) == 0) {
						mUser = &user;
						net::http::PayloadWriter response;
						response << 0x0;
						mSocket->write(response.buildPayload());
						return;
					}
				}
				user::manager::unloadUser(userID);
			}
			return;
		}
		switch (id) {
			case 0x1:
				processFriendListRequestPacket(payload);
				break;
			case 0x2:
				processUsernameRequestPacket(payload);
				break;
			case 0x3:
				processAcceptFriendRequestPacket(payload);
				break;
			case 0x4:
				processChannelListRequestPacket(payload);
				break;
			case 0x5:
				processMessagePacket(payload);
				break;
			case 0x6:
				processChannelContentsRequestPacket(payload);
				break;
			case 0x7:
				processCreateChannelPacket(payload);
				break;
			default:
				std::cout << "Unknown packet ID: " << id << std::endl;
				break;
		}
	}

	void Client::processFriendListRequestPacket(net::http::PayloadReader&) const {
		net::http::PayloadWriter response;
		response << 0x1;
		const std::vector<user::manager::UserID>& friends = mUser->getFriends();
		response << friends.size();
		for (const user::manager::UserID& friendID : friends) {
			response << friendID;
			response << user::manager::loadUser(friendID).getUsername();
			user::manager::unloadUser(friendID);
		}
		mSocket->write(response.buildPayload());
	}

	void Client::processUsernameRequestPacket(net::http::PayloadReader& payload) const {
		const user::manager::UserID userID = payload.readUserID();
		user::User& user = user::manager::loadUser(userID);
		const std::string& username = user.getUsername();
		net::http::PayloadWriter response;
		response << 0x2;
		response << userID;
		response << username;
		mSocket->write(response.buildPayload());
		user::manager::unloadUser(userID);
	}

	void Client::processAcceptFriendRequestPacket(net::http::PayloadReader& payload) const {
		const user::manager::UserID userID = payload.readUserID();
		mUser->getFriends().push_back(userID);
		user::User& user = user::manager::loadUser(userID);
		user.getFriends().push_back(mUser->getID());
		user::manager::unloadUser(userID);
	}

	void Client::processChannelListRequestPacket(net::http::PayloadReader& payload) const {
		const user::manager::UserID userID = payload.readUserID();
		user::User& user = user::manager::loadUser(userID);
		const std::vector<chat::ChannelID>& channels = user.getChannels();
		net::http::PayloadWriter response;
		response << 0x3;
		response << userID;
		response << channels.size();
		for (const chat::ChannelID& c: channels) {
			response << c;
			response << chat::Channel(c).getName();
		}
		user::manager::unloadUser(userID);
		mSocket->write(response.buildPayload());
	}

	void Client::processMessagePacket(net::http::PayloadReader& payload) const {
		const chat::ChannelID channelID = payload.readChannelID();
		std::string message;
		payload >> message;
		chat::Channel channel(channelID);
		channel.getMessages().push_back({.content = message, .from = mUser->getUsername(), .fromID = mUser->getID(), .channelID = channelID});
		channel.save();
		for (user::manager::UserID& userID : channel.getUsers()) {
			if (!user::manager::isActive(userID)) {
				continue;
			}
			user::User& user = user::manager::loadUser(userID);
			net::http::PayloadWriter messagePayload;
			messagePayload << 0x4;
			messagePayload << mUser->getID();
			messagePayload << channelID;
			messagePayload << message;
			messagePayload << mUser->getUsername();
			user.pushPayload(messagePayload.buildAllocatedPayload());
			user::manager::unloadUser(userID);
		}
	}

	void Client::processChannelContentsRequestPacket(net::http::PayloadReader& payload) const {
		const chat::ChannelID channelID = payload.readChannelID();
		chat::Channel channel(channelID);
		net::http::PayloadWriter response;
		response << 0x5;
		response << channelID;
		response << channel.getName();
		const std::vector<chat::Message>& messages = channel.getMessages();
		const uint32_t messageCount = messages.size();
		response << messageCount;
		for (uint32_t i = 0; i < messageCount; i++) {
			response << messages[i].fromID;
			response << messages[i].content;
			response << messages[i].from;
		}
		mSocket->write(response.buildPayload());
	}

	void Client::processCreateChannelPacket(net::http::PayloadReader& payload) {
		const chat::ChannelID channelID;
		chat::Channel channel(channelID);
		std::string channelName;
		payload >> channelName;
		channel.setName(channelName);
		uint32_t userCount;
		payload >> userCount;
		for (uint32_t i = 0; i < userCount; i++) {
			user::manager::UserID userID = payload.readUserID();
			channel.getUsers().push_back(userID);
			user::User& user = user::manager::loadUser(userID);
			user.getChannels().push_back(channelID);
			user::manager::unloadUser(userID);
		}
		channel.save();
	}
}
