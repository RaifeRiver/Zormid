#include "Channel.h"

#include <cstring>
#include <filesystem>
#include <random>

#include "Message.h"
#include "user/User.h"
#include "util/Base16.h"
#include "util/FileStream.h"

namespace zormid::chat {
	ChannelID::ChannelID() {
		std::string hash;
		do {
			std::random_device randomDevice;
			for (char& c : mID) {
				c = static_cast<char>(randomDevice());
			}
			hash = util::base16::encode(mID, 20);
		}
		while (std::filesystem::exists(CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".dat"));
	}

	ChannelID::ChannelID(const char* id) {
		memcpy(mID, id, 20);
	}

	Channel::Channel(const ChannelID& id) : mID(id) {
		const std::string hash = util::base16::encode(static_cast<const char*>(id), 20);
		const std::string dir = hash.substr(0, 2);
		if (!std::filesystem::exists(CHANNELS_DIR + dir)) {
			return;
		}
		const std::string file = CHANNELS_DIR + dir + "/" + hash.substr(2) + ".dat";
		if (!std::filesystem::exists(file)) {
			return;
		}
		util::IFileStream fileStream(file);
		fileStream >> mName;
		uint32_t messageCount;
		fileStream >> messageCount;
		mMessages.reserve(messageCount);
		for (uint32_t i = 0; i < messageCount; i++) {
			char userIDChars[20];
			fileStream.read(userIDChars, 20);
			user::manager::UserID fromID(userIDChars);
			std::string content;
			fileStream >> content;
			const std::string& username = user::manager::loadUser(fromID).getUsername();
			mMessages.push_back({.content = content, .from = username, .fromID = fromID, .channelID = mID});
			user::manager::unloadUser(fromID);
		}
		uint32_t userCount;
		fileStream >> userCount;
		for (uint32_t i = 0; i < userCount; i++) {
			char userIDChars[20];
			fileStream.read(userIDChars, 20);
			mUsers.emplace_back(userIDChars);
		}
	}

	void Channel::send(const Message& message) {
		mMessages.push_back(message);
	}

	void Channel::setName(const std::string& name) {
		mName = name;
	}

	void Channel::save() const {
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		std::filesystem::create_directories(CHANNELS_DIR + hash.substr(0, 2));
		util::OFileStream fileStream(CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".dat");
		fileStream << mName;
		fileStream << mMessages.size();
		for (const Message& message : mMessages) {
			fileStream.write(static_cast<const char*>(message.fromID), 20);
			fileStream << message.content;
		}
		fileStream << mUsers.size();
		for (const user::manager::UserID& userID : mUsers) {
			fileStream.write(static_cast<const char*>(userID), 20);
		}
	}
}
