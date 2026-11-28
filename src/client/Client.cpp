#include "Client.h"

#include <cstring>
#include <filesystem>
#include <iostream>

#include "Zormid.h"
#include "chat/CallManager.h"
#include "chat/VoiceChannel.h"
#include "util/Base64.h"
#include "util/FileStream.h"

namespace zormid::client {
	Client::Client(std::unique_ptr<net::http::WebSocket> socket) : mSocket(std::move(socket)) {}

	void Client::process() {
		std::optional<net::http::PayloadData> payload;
		while ((payload = mSocket->read()).has_value()) {
			net::http::PayloadReader payloadReader(*payload);
			processPacket(payloadReader);
		}
		if (!mSocket->isOpen() && mUser) {
			close_();
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
			close_();
		}
	}

	bool Client::isOpen() {
		const bool open = mSocket->isOpen();
		if (!open && mUser) {
			close_();
		}
		return open;
	}

	void Client::sendUserInfoToAll() const {
		for (const user::manager::UserID& friendID : mUser->getFriends()) {
			if (!user::manager::isActive(friendID)) {
				continue;
			}
			user::User& user = user::manager::loadUser(friendID);
			if (user.getActiveClients() == 0) {
				user::manager::unloadUser(friendID);
				continue;
			}
			net::http::PayloadWriter response;
			response << 0x9u;
			response << mUser->getID();
			response << mUser->getUsername();
			response << mUser->getAvatar();
			response << (mUser->getActiveClients() == 0 ? 0u : 1u);
			user.pushPayload(response.buildAllocatedPayload());
			user::manager::unloadUser(friendID);
		}
		for (const chat::ChannelID channelID : mUser->getChannels()) {
			for (const std::unique_ptr<chat::Channel> channel = chat::loadChannel(channelID); const user::manager::UserID& userID : channel->getUsers()) {
				if (userID == mUser->getID()) {
					continue;
				}
				if (!user::manager::isActive(userID)) {
					continue;
				}
				user::User& user = user::manager::loadUser(userID);
				if (user.getActiveClients() == 0) {
					user::manager::unloadUser(userID);
					continue;
				}
				net::http::PayloadWriter response;
				response << 0x9u;
				response << mUser->getID();
				response << mUser->getUsername();
				response << mUser->getAvatar();
				response << (mUser->getActiveClients() == 0 ? 0u : 1u);
				user.pushPayload(response.buildAllocatedPayload());
				user::manager::unloadUser(userID);
			}
		}
		net::http::PayloadWriter response;
		response << 0x9u;
		response << mUser->getID();
		response << mUser->getUsername();
		response << mUser->getAvatar();
		response << (mUser->getActiveClients() == 0 ? 0u : 1u);
		mUser->pushPayload(response.buildAllocatedPayload());
	}

	void Client::sendTypingToAllInChannels() const {
		for (const chat::ChannelID channelID : mUser->getChannels()) {
			for (const std::unique_ptr<chat::Channel> channel = chat::loadChannel(channelID); const user::manager::UserID& userID : channel->getUsers()) {
				if (userID == mUser->getID()) {
					continue;
				}
				if (!user::manager::isActive(userID)) {
					continue;
				}
				user::User& user = user::manager::loadUser(userID);
				if (user.getActiveClients() == 0) {
					user::manager::unloadUser(userID);
					continue;
				}
				net::http::PayloadWriter response;
				response << 0xCu;
				response << (mTypingChannel.has_value() ? *mTypingChannel : channelID);
				response << mUser->getID();
				response << mTypingChannel.has_value();
				user.pushPayload(response.buildAllocatedPayload());
				user::manager::unloadUser(userID);
			}
		}
	}

	void Client::close_() {
		if (mTypingChannel.has_value()) {
			mTypingChannel = {};
			if (--mUser->getTypingClients() == 0) {
				sendTypingToAllInChannels();
			}
		}
		if (mCallChannel) {
			processLeaveCallPacket();
		}
		mUser->getActiveClients()--;
		sendUserInfoToAll();
		user::manager::unloadUser(mUser->getID());
		mUser = nullptr;
	}

	void Client::processPacket(net::http::PayloadReader& payload) {
		uint32_t id = 0;
		payload >> id;
		if (!mUser) {
			if (id == 0x0) {
				processAuthPacket(payload);
			}
			return;
		}
		switch (id) {
			case 0x1:
				processFriendListRequestPacket();
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
				processChannelInfoRequestPacket(payload);
				break;
			case 0x7:
				processCreateChannelPacket(payload);
				break;
			case 0x8:
				processChannelMessageRequestPacket(payload);
				break;
			case 0x9:
				processChangeUsernamePacket(payload);
				break;
			case 0xA:
				processChangeChannelUsersPacket(payload);
				break;
			case 0xB:
				processFileUploadRequestPacket(payload);
				break;
			case 0xC:
				processFileUploadPacket(payload);
				break;
			case 0xD:
				processSessionsRequestPacket();
				break;
			case 0xE:
				processLogoutSessionPacket(payload);
				break;
			case 0xF:
				processSetAvatarPacket(payload);
				break;
			case 0x10:
				processTypingPacket(payload);
				break;
			case 0x11:
				processJoinCallPacket(payload);
				break;
			case 0x12:
				processCallOfferPacket(payload);
				break;
			case 0x13:
				processCallCandidatePacket(payload);
				break;
			case 0x14:
				processLeaveCallPacket();
				break;
			default:
				std::cout << "Unknown packet ID: " << id << std::endl;
				break;
		}
	}

	void Client::processAuthPacket(net::http::PayloadReader& payload) {
		const user::manager::UserID userID = payload.readUserID();
		const user::Session sessionID = payload.readSession();
		if (!user::manager::exists(userID)) {
			close();
			return;
		}
		for (user::User& user = user::manager::loadUser(userID); const user::Session& session : user.getSessions()) {
			if (memcmp(static_cast<const char*>(sessionID), static_cast<const char*>(session), 16) == 0) {
				mUser = &user;
				mUser->getActiveClients()++;
				net::http::PayloadWriter response;
				response << 0x0u;
				response << mUser->getUsername();
				response << mUser->getAvatar();
				mSocket->write(response.buildPayload());
				sendUserInfoToAll();
				return;
			}
		}
		user::manager::unloadUser(userID);
	}

	void Client::processFriendListRequestPacket() const {
		net::http::PayloadWriter response;
		response << 0x1u;
		const std::vector<user::manager::UserID>& friends = mUser->getFriends();
		response << static_cast<uint32_t>(friends.size());
		for (const user::manager::UserID& friendID : friends) {
			response << friendID;
			user::User& user = user::manager::loadUser(friendID);
			response << user.getUsername();
			response << user.getAvatar();
			response << (user.getActiveClients() > 0 ? 1u : 0u);
			user::manager::unloadUser(friendID);
		}
		mSocket->write(response.buildPayload());
	}

	void Client::processUsernameRequestPacket(net::http::PayloadReader& payload) const {
		const user::manager::UserID userID = payload.readUserID();
		user::User& user = user::manager::loadUser(userID);
		const std::string& username = user.getUsername();
		net::http::PayloadWriter response;
		response << 0x2u;
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
		if (user.getActiveClients() != 0) {
			net::http::PayloadWriter response;
			response << 0x1u;
			const std::vector<user::manager::UserID>& friends = user.getFriends();
			response << static_cast<uint32_t>(friends.size());
			for (const user::manager::UserID& friendID : friends) {
				response << friendID;
				user::User& user2 = user::manager::loadUser(friendID);
				response << user2.getUsername();
				response << user2.getAvatar();
				response << (user2.getActiveClients() > 0 ? 1u : 0u);
				user::manager::unloadUser(friendID);
			}
			user.pushPayload(response.buildAllocatedPayload());
		}
		user::manager::unloadUser(userID);
		net::http::PayloadWriter response;
		response << 0x1u;
		const std::vector<user::manager::UserID>& friends = mUser->getFriends();
		response << static_cast<uint32_t>(friends.size());
		for (const user::manager::UserID& friendID : friends) {
			response << friendID;
			user::User& user2 = user::manager::loadUser(friendID);
			response << user2.getUsername();
			response << user2.getAvatar();
			response << (user2.getActiveClients() > 0 ? 1u : 0u);
			user::manager::unloadUser(friendID);
		}
		mUser->pushPayload(response.buildAllocatedPayload());
	}

	void Client::processChannelListRequestPacket(net::http::PayloadReader& payload) const {
		const user::manager::UserID userID = payload.readUserID();
		user::User& user = user::manager::loadUser(userID);
		const std::vector<chat::ChannelID>& channels = user.getChannels();
		net::http::PayloadWriter response;
		response << 0x3u;
		response << userID;
		response << static_cast<uint32_t>(channels.size());
		for (const chat::ChannelID& c: channels) {
			response << c;
			const std::unique_ptr<chat::Channel> channel = chat::loadChannel(c);
			response << static_cast<uint32_t>(channel->getType());
			response << channel->getName();
		}
		user::manager::unloadUser(userID);
		mSocket->write(response.buildPayload());
	}

	void Client::processMessagePacket(net::http::PayloadReader& payload) const {
		const chat::ChannelID channelID = payload.readChannelID();
		std::string message;
		payload >> message;
		const int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		const std::unique_ptr<chat::Channel> channel = chat::loadChannel(channelID);
		auto* textChannel = dynamic_cast<chat::TextChannel*>(channel.get());
		if (!textChannel) {
			return;
		}
		const uint32_t index = textChannel->send({.content = message, .from = mUser->getUsername(), .fromID = mUser->getID(), .channelID = channelID, .timestamp = timestamp});
		for (const user::manager::UserID& userID : textChannel->getUsers()) {
			if (!user::manager::isActive(userID)) {
				continue;
			}
			user::User& user = user::manager::loadUser(userID);
			if (user.getActiveClients() == 0) {
				user::manager::unloadUser(userID);
				continue;
			}
			net::http::PayloadWriter messagePayload;
			messagePayload << 0x4u;
			messagePayload << mUser->getID();
			messagePayload << channelID;
			messagePayload << message;
			messagePayload << mUser->getUsername();
			messagePayload << index;
			messagePayload << timestamp;
			user.pushPayload(messagePayload.buildAllocatedPayload());
			user::manager::unloadUser(userID);
		}
	}

	void Client::processChannelInfoRequestPacket(net::http::PayloadReader& payload) const {
		const chat::ChannelID channelID = payload.readChannelID();
		const std::unique_ptr<chat::Channel> channel = chat::loadChannel(channelID);
		net::http::PayloadWriter response;
		response << 0x5u;
		response << channelID;
		response << static_cast<uint32_t>(channel->getType());
		response << channel->getName();
		const std::vector<user::manager::UserID>& users = channel->getUsers();
		response << static_cast<uint32_t>(users.size());
		for (const user::manager::UserID& userID : users) {
			response << userID;
			user::User& user = user::manager::loadUser(userID);
			response << user.getUsername();
			response << user.getAvatar();
			response << (user.getActiveClients() > 0 ? 1u : 0u);
			user::manager::unloadUser(userID);
		}
		if (channel->getType() == chat::ChannelType::TEXT) {
			const auto* textChannel = dynamic_cast<chat::TextChannel*>(channel.get());
			response << textChannel->getMessageCount();
		}
		mSocket->write(response.buildPayload());
	}

	void Client::processChannelMessageRequestPacket(net::http::PayloadReader& payload) const {
		const chat::ChannelID channelID = payload.readChannelID();
		const std::unique_ptr<chat::Channel> channel = chat::loadChannel(channelID);
		const auto* textChannel = dynamic_cast<chat::TextChannel*>(channel.get());
		if (!textChannel) {
			return;
		}
		uint32_t firstMessage;
		payload >> firstMessage;
		net::http::PayloadWriter response;
		response << 0x6u;
		response << channelID;
		response << firstMessage;
		const uint32_t messageCount = std::min(textChannel->getMessageCount() - firstMessage, 50u);
		response << messageCount;
		for (const chat::Message& message : textChannel->getMessages(firstMessage, messageCount)) {
			response << message.fromID;
			response << message.content;
			response << message.from;
			response << message.timestamp;
		}
		mSocket->write(response.buildPayload());
	}

	void Client::processChangeUsernamePacket(net::http::PayloadReader& payload) const {
		std::string username;
		payload >> username;
		user::manager::changeUsername(mUser->getID(), username);
		sendUserInfoToAll();
	}

	void Client::processFileUploadRequestPacket(net::http::PayloadReader& payload) const {
		char id[20];
		payload.read(id, 20);
		uint32_t size;
		payload >> size;
		const std::string encodedID = util::base64::encodeURL(id, 20);
		net::http::PayloadWriter response;
		response << 0x7u;
		response.write(id, 20);
		if (const int64_t maxUploadSize = Zormid::getInstance().getConfig().getMaxUploadSize(); maxUploadSize != -1 && size > maxUploadSize) {
			response << 1u;
		}
		else if (std::filesystem::exists("../data/uploads/" + encodedID.substr(0, 2) + "/" + encodedID.substr(2))) {
			response << 2u;
		}
		else {
			response << 0u;
		}
		mSocket->write(response.buildPayload());
	}

	void Client::processSessionsRequestPacket() const {
		const std::vector<user::Session>& sessions = mUser->getSessions();
		net::http::PayloadWriter response;
		response << 0x8u;
		response << static_cast<uint32_t>(sessions.size());
		for (const user::Session& session : sessions) {
			response << session.getUserAgent();
		}
		mSocket->write(response.buildPayload());
	}

	void Client::processLogoutSessionPacket(net::http::PayloadReader& payload) const {
		std::string userAgent;
		payload >> userAgent;
		std::erase_if(mUser->getSessions(), [&userAgent](const user::Session& session) {
			return session.getUserAgent() == userAgent;
		});
	}

	void Client::processSetAvatarPacket(net::http::PayloadReader& payload) const {
		std::string avatar;
		payload >> avatar;
		mUser->setAvatar(avatar);
		sendUserInfoToAll();
	}

	void Client::processTypingPacket(net::http::PayloadReader& payload) {
		bool typing;
		payload >> typing;
		chat::ChannelID channelID = payload.readChannelID();
		if (typing != mTypingChannel.has_value()) {
			mTypingChannel = typing ? channelID : std::optional<chat::ChannelID>{};
			if (typing) {
				if (mUser->getTypingClients()++ == 0) {
					sendTypingToAllInChannels();
				}
			}
			else if (--mUser->getTypingClients() == 0) {
				sendTypingToAllInChannels();
			}
		}
	}

	void Client::processJoinCallPacket(net::http::PayloadReader& payload) {
		const chat::ChannelID channelID = payload.readChannelID();
		std::vector<Client*>& clientsInCall = chat::call::getClientsInCall(channelID);
		net::http::PayloadWriter response;
		response << 0xDu;
		response << mUser->getID();
		const net::http::PayloadData data = response.buildPayload();
		for (const Client* client : clientsInCall) {
			client->getSocket().write(data);
		}
		clientsInCall.push_back(this);
		mCallChannel = channelID;
	}

	void Client::processCallOfferPacket(net::http::PayloadReader& payload) const {
		const user::manager::UserID userID = payload.readUserID();
		bool responseOffer;
		payload >> responseOffer;
		std::string offer;
		payload >> offer;
		net::http::PayloadWriter response;
		response << 0xEu;
		response << responseOffer;
		response << mUser->getID();
		response << offer;
		const net::http::PayloadData data = response.buildPayload();
		for (const Client* client : chat::call::getClientsInCall(*mCallChannel)) {
			if (client->getUserID() == userID) {
				client->getSocket().write(data);
			}
		}
	}

	void Client::processCallCandidatePacket(net::http::PayloadReader& payload) const {
		const user::manager::UserID userID = payload.readUserID();
		std::string candidate;
		payload >> candidate;
		std::string sdpMid;
		payload >> sdpMid;
		uint32_t sdpMLineIndex;
		payload >> sdpMLineIndex;
		net::http::PayloadWriter response;
		response << 0xFu;
		response << candidate;
		response << sdpMid;
		response << sdpMLineIndex;
		const net::http::PayloadData data = response.buildPayload();
		for (const Client* client : chat::call::getClientsInCall(*mCallChannel)) {
			if (client->getUserID() == userID) {
				client->getSocket().write(data);
			}
		}
	}

	void Client::processLeaveCallPacket() {
		if (!mCallChannel) {
			return;
		}
		chat::call::leaveCall(*mCallChannel, this);
		net::http::PayloadWriter response;
		response << 0x10u;
		response << *mCallChannel;
		response << mUser->getID();
		const net::http::PayloadData data = response.buildPayload();
		for (const Client* client : chat::call::getClientsInCall(*mCallChannel)) {
			client->getSocket().write(data);
		}
		mCallChannel = {};
	}

	void Client::processFileUploadPacket(net::http::PayloadReader& payload) {
		char id[20];
		payload.read(id, 20);
		std::string name;
		payload >> name;
		uint32_t size;
		payload >> size;
		if (int64_t maxUploadSize = Zormid::getInstance().getConfig().getMaxUploadSize(); maxUploadSize != -1 && size > maxUploadSize) {
			return;
		}
		const std::string encodedID = util::base64::encodeURL(id, 20);
		const std::string dir = "../data/uploads/" + encodedID.substr(0, 2);
		if (!std::filesystem::exists(dir)) {
			std::filesystem::create_directories(dir);
		}
		std::string file = dir + "/" + encodedID.substr(2);
		util::OFileStream fileStream(file);
		fileStream << name;
		fileStream << size;
		auto contents = static_cast<char*>(malloc(size));
		payload.read(contents, size);
		fileStream.write(contents, size);
		free(contents);
	}

	void Client::processChangeChannelUsersPacket(net::http::PayloadReader& payload) {
		const chat::ChannelID channelID = payload.readChannelID();
		const std::unique_ptr<chat::Channel> channel = chat::loadChannel(channelID);
		uint32_t userCount;
		payload >> userCount;
		std::vector<user::manager::UserID> oldUsers = channel->getUsers();
		for (uint32_t i = 0; i < userCount; i++) {
			user::manager::UserID userID = payload.readUserID();
			if (auto it = std::ranges::find(oldUsers, userID); it == oldUsers.end()) {
				channel->addUser(userID);
				user::User& user = user::manager::loadUser(userID);
				if (user.getActiveClients() != 0) {
					net::http::PayloadWriter response;
					response << 0xAu;
					response << channelID;
					response << channel->getName();
					user.pushPayload(response.buildAllocatedPayload());
				}
				user.getChannels().push_back(channelID);
				user::manager::unloadUser(userID);
			}
			else {
				oldUsers.erase(it);
			}
		}
		for (const user::manager::UserID& userID : oldUsers) {
			channel->removeUser(userID);
			user::User& user = user::manager::loadUser(userID);
			if (user.getActiveClients() != 0) {
				net::http::PayloadWriter response;
				response << 0xBu;
				response << channelID;
				user.pushPayload(response.buildAllocatedPayload());
			}
			user.getChannels().erase(std::ranges::find(user.getChannels(), channelID));
			user::manager::unloadUser(userID);
		}
		if (channel->getUsers().empty()) {
			channel->deleteChannel();
		}
		else {
			for (const user::manager::UserID& userID : channel->getUsers()) {
				user::User& user = user::manager::loadUser(userID);
				if (user.getActiveClients() == 0) {
					user::manager::unloadUser(userID);
					continue;
				}
				net::http::PayloadWriter response;
				response << 0x5u;
				response << channelID;
				response << static_cast<uint32_t>(channel->getType());
				response << channel->getName();
				const std::vector<user::manager::UserID>& users = channel->getUsers();
				response << static_cast<uint32_t>(users.size());
				for (const user::manager::UserID& userID2 : users) {
					response << userID2;
					user::User& user2 = user::manager::loadUser(userID2);
					response << user2.getUsername();
					response << user2.getAvatar();
					response << (user2.getActiveClients() > 0 ? 1u : 0u);
					user::manager::unloadUser(userID2);
				}
				if (channel->getType() == chat::ChannelType::TEXT) {
					const auto* textChannel = dynamic_cast<chat::TextChannel*>(channel.get());
					response << textChannel->getMessageCount();
				}
				user.pushPayload(response.buildAllocatedPayload());
				user::manager::unloadUser(userID);
			}
		}
	}

	void Client::processCreateChannelPacket(net::http::PayloadReader& payload) {
		chat::ChannelType channelType;
		payload >> reinterpret_cast<uint32_t&>(channelType);
		const chat::ChannelID channelID;
		std::unique_ptr<chat::Channel> channel;
		switch (channelType) {
			case chat::ChannelType::TEXT:
				channel = std::make_unique<chat::TextChannel>(channelID);
				break;
			case chat::ChannelType::VOICE:
				channel = std::make_unique<chat::VoiceChannel>(channelID);
				break;
			default:
				return;
		}
		channel->create();
		std::string channelName;
		payload >> channelName;
		channel->setName(channelName);
		uint32_t userCount;
		payload >> userCount;
		for (uint32_t i = 0; i < userCount; i++) {
			user::manager::UserID userID = payload.readUserID();
			channel->addUser(userID);
			user::User& user = user::manager::loadUser(userID);
			user.getChannels().push_back(channelID);
			if (user.getActiveClients() != 0) {
				net::http::PayloadWriter response;
				response << 0xAu;
				response << channelID;
				response << static_cast<uint32_t>(channelType);
				response << channelName;
				user.pushPayload(response.buildAllocatedPayload());
			}
			user::manager::unloadUser(userID);
		}
	}
}
