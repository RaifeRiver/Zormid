#include "Client.h"

#include <cstring>
#include <iostream>

#include "Zormid.h"

namespace zormid::client {
	Client::Client(std::unique_ptr<net::http::WebSocket> socket) : mSocket(std::move(socket)) {}

	void Client::sendMessage(const chat::Message& message, const bool global) const {
		if (!mUser.has_value()) {
			return;
		}
		net::http::PayloadWriter payload;
		payload << (global ? 0x1 : 0x4);
		payload << message.fromID;
		payload << message.channelID;
		payload << message.content;
		payload << message.from;
		mSocket->write(payload.buildPayload());
	}

	void Client::process() {
		std::optional<net::http::PayloadData> payload;
		while ((payload = mSocket->read()).has_value()) {
			net::http::PayloadReader payloadReader(*payload);
			processPacket(payloadReader);
		}
	}

	void Client::close() const {
		mSocket->close();
	}

	bool Client::isOpen() const {
		return mSocket->isOpen();
	}

	void Client::processPacket(net::http::PayloadReader& payload) {
		uint32_t id = 0;
		payload >> id;
		if (!mUser.has_value()) {
			if (id == 0x0) {
				const user::manager::UserID userID = payload.readUserID();
				const user::Session sessionID = payload.readSession();
				for (user::User user = user::manager::loadUser(userID); user::Session session : user.getSessions()) {
					if (memcmp(static_cast<const char*>(sessionID), static_cast<const char*>(session), 16) == 0) {
						mUser = user;
						net::http::PayloadWriter response;
						response << 0x0;
						mSocket->write(response.buildPayload());
						return;
					}
				}
			}
			return;
		}
		switch (id) {
			case 0x1:
				processGlobalMessagePacket(payload);
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
			default:
				std::cout << "Unknown packet ID: " << id << std::endl;
				break;
		}
	}

	void Client::processGlobalMessagePacket(net::http::PayloadReader& payload) {
		std::string message;
		payload >> message;
		for (Client& c : Zormid::getInstance().getClients()) {
			c.sendMessage({.content = message, .from = mUser->getUsername(), .fromID = mUser->getID()}, true);
		}
	}

	void Client::processUsernameRequestPacket(net::http::PayloadReader& payload) const {
		const user::manager::UserID userID = payload.readUserID();
		user::User user = user::manager::loadUser(userID);
		const std::string& username = user.getUsername();
		net::http::PayloadWriter response;
		response << 0x2;
		response << userID;
		response << username;
		mSocket->write(response.buildPayload());
	}

	void Client::processAcceptFriendRequestPacket(net::http::PayloadReader& payload) {
		const user::manager::UserID userID = payload.readUserID();
		user::User user = user::manager::loadUser(userID);
		user.getFriends().push_back(mUser->getID());
		const chat::ChannelID channelID;
		chat::Channel channel(channelID);
		channel.setName(mUser->getUsername() + " & " + user.getUsername());
		channel.getUsers().push_back(mUser->getID());
		channel.getUsers().push_back(userID);
		channel.save();
		user.getChannels().push_back(channelID);
		user::manager::saveUser(user);
		for (Client& client : Zormid::getInstance().getClients()) {
			if (client.getUserID() == userID || client.getUserID() == mUser->getID()) {
				client.getUser()->getFriends().push_back(userID);
				client.getUser()->getChannels().push_back(channelID);
				user::manager::saveUser(*client.getUser());
				const std::vector<chat::ChannelID>& channels = user.getChannels();
				net::http::PayloadWriter response;
				response << 0x3;
				response << userID;
				response << channels.size();
				for (const chat::ChannelID& c: channels) {
					std::string channelName = chat::Channel(c).getName();
					const uint32_t channelNameLength = channelName.length();
					response << c;
					response << channelNameLength;
					response << channelName;
				}
				client.getSocket().write(response.buildPayload());
			}
		}
	}

	void Client::processChannelListRequestPacket(net::http::PayloadReader& payload) const {
		const user::manager::UserID userID = payload.readUserID();
		user::User user = user::manager::loadUser(userID);
		const std::vector<chat::ChannelID>& channels = user.getChannels();
		net::http::PayloadWriter response;
		response << 0x3;
		response << userID;
		response << channels.size();
		for (const chat::ChannelID& c: channels) {
			std::string channelName = chat::Channel(c).getName();
			const uint32_t channelNameLength = channelName.length();
			response << c;
			response << channelNameLength;
			response << channelName;
		}
		mSocket->write(response.buildPayload());
	}

	void Client::processMessagePacket(net::http::PayloadReader& payload) {
		const chat::ChannelID channelID = payload.readChannelID();
		std::string message;
		payload >> message;
		chat::Channel channel(channelID);
		channel.getMessages().push_back({.content = message, .from = mUser->getUsername(), .fromID = mUser->getID(), .channelID = channelID});
		channel.save();
		for (Client& client : Zormid::getInstance().getClients()) {
			bool found = false;
			for (const chat::ChannelID& c : client.getUser()->getChannels()) {
				if (c == channelID) {
					found = true;
					break;
				}
			}
			if (found) {
				client.sendMessage({.content = message, .from = mUser->getUsername(), .fromID = mUser->getID(), .channelID = channelID});
			}
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
}
