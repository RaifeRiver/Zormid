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
		net::http::PayloadData payload = {};
		payload.type = net::http::PayloadType::BINARY;
		payload.size = message.content.size() + message.from.size() + 52;
		payload.data = static_cast<char*>(malloc(payload.size));
		const uint32_t packetID = global ? 0x1 : 0x4;
		memcpy(payload.data, &packetID, 4);
		memcpy(payload.data + 4, static_cast<const char*>(message.fromID), 20);
		memcpy(payload.data + 24, static_cast<const char*>(message.channelID), 20);
		const uint32_t messageLength = message.content.size();
		memcpy(payload.data + 44, &messageLength, 4);
		memcpy(payload.data + 48, message.content.data(), message.content.size());
		const uint32_t fromLength = message.from.size();
		memcpy(payload.data + 48 + message.content.size(), &fromLength, 4);
		memcpy(payload.data + 52 + message.content.size(), message.from.data(), message.from.size());
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
		uint32_t id = 0;
		memcpy(&id, payload.data, 4);
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
				break;
		}
	}

	void Client::processGlobalMessagePacket(const net::http::PayloadData& payload) {
		uint32_t length = 0;
		memcpy(&length, payload.data + 4, 4);
		std::string message(length, 0);
		for (uint32_t i = 0; i < length; i++) {
			message[i] = payload.data[i + 8];
		}
		for (Client& c : Zormid::getInstance().getClients()) {
			c.sendMessage({.content = message, .from = mUser->getUsername(), .fromID = mUser->getID()}, true);
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
				net::http::PayloadData response = {};
				response.type = net::http::PayloadType::BINARY;
				std::vector<char> responseData = {};
				constexpr uint32_t responseID = 0x3;
				responseData.resize(28);
				memcpy(responseData.data(), &responseID, 4);
				memcpy(responseData.data() + 4, userIDChars, 20);
				const uint32_t channelCount = channels.size();
				memcpy(responseData.data() + 24, &channelCount, 4);
				size_t pos = 28;
				for (uint32_t i = 0; i < channelCount; i++) {
					std::string channelName = chat::Channel(channels[i]).getName();
					const uint32_t channelNameLength = channelName.length();
					responseData.resize(responseData.size() + 24 + channelNameLength);
					memcpy(responseData.data() + pos, static_cast<const char*>(channels[i]), 20);
					memcpy(responseData.data() + pos + 20, &channelNameLength, 4);
					memcpy(responseData.data() + pos + 24, channelName.c_str(), channelNameLength);
					pos += channelNameLength + 24;
				}
				response.data = responseData.data();
				response.size = responseData.size();
				client.getSocket().write(response);
			}
		}
	}

	void Client::processChannelListRequestPacket(const net::http::PayloadData& payload) const {
		char userIDChars[20];
		memcpy(userIDChars, payload.data + 4, 20);
		const user::manager::UserID userID(userIDChars);
		user::User user = user::manager::loadUser(userID);
		const std::vector<chat::ChannelID>& channels = user.getChannels();
		net::http::PayloadData response = {};
		response.type = net::http::PayloadType::BINARY;
		std::vector<char> responseData = {};
		constexpr uint32_t responseID = 0x3;
		responseData.resize(28);
		memcpy(responseData.data(), &responseID, 4);
		memcpy(responseData.data() + 4, userIDChars, 20);
		const uint32_t channelCount = channels.size();
		memcpy(responseData.data() + 24, &channelCount, 4);
		size_t pos = 28;
		for (uint32_t i = 0; i < channelCount; i++) {
			std::string channelName = chat::Channel(channels[i]).getName();
			const uint32_t channelNameLength = channelName.length();
			responseData.resize(responseData.size() + 24 + channelNameLength);
			memcpy(responseData.data() + pos, static_cast<const char*>(channels[i]), 20);
			memcpy(responseData.data() + pos + 20, &channelNameLength, 4);
			memcpy(responseData.data() + pos + 24, channelName.c_str(), channelNameLength);
			pos += channelNameLength + 24;
		}
		response.data = responseData.data();
		response.size = responseData.size();
		mSocket->write(response);
	}

	void Client::processMessagePacket(const net::http::PayloadData& payload) {
		char channelIDChars[20];
		memcpy(channelIDChars, payload.data + 4, 20);
		const chat::ChannelID channelID(channelIDChars);
		uint32_t length = 0;
		memcpy(&length, payload.data + 24, 4);
		std::string message(length, 0);
		for (uint32_t i = 0; i < length; i++) {
			message[i] = payload.data[i + 28];
		}
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

	void Client::processChannelContentsRequestPacket(const net::http::PayloadData& payload) const {
		char channelIDChars[20];
		memcpy(channelIDChars, payload.data + 4, 20);
		const chat::ChannelID channelID(channelIDChars);
		chat::Channel channel(channelID);
		net::http::PayloadData response = {};
		response.type = net::http::PayloadType::BINARY;
		std::vector<char> responseData = {};
		responseData.resize(28);
		constexpr uint32_t responseID = 0x5;
		memcpy(responseData.data(), &responseID, 4);
		memcpy(responseData.data() + 4, channelIDChars, 20);
		const std::vector<chat::Message>& messages = channel.getMessages();
		const uint32_t messageCount = messages.size();
		memcpy(responseData.data() + 24, &messageCount, 4);
		size_t pos = 28;
		for (uint32_t i = 0; i < messageCount; i++) {
			const std::string& content = messages[i].content;
			const std::string& username = messages[i].from;
			auto contentLength = static_cast<uint32_t>(content.length());
			auto usernameLength = static_cast<uint32_t>(username.length());
			responseData.resize(responseData.size() + 28 + contentLength + usernameLength);
			memcpy(responseData.data() + pos, static_cast<const char*>(messages[i].fromID), 20);
			pos += 20;
			memcpy(responseData.data() + pos, &contentLength, 4);
			pos += 4;
			memcpy(responseData.data() + pos, content.data(), contentLength);
			pos += contentLength;
			memcpy(responseData.data() + pos, &usernameLength, 4);
			pos += 4;
			memcpy(responseData.data() + pos, username.data(), usernameLength);
			pos += usernameLength;
		}
		response.data = responseData.data();
		response.size = responseData.size();
		mSocket->write(response);
	}
}
