#include "Channel.h"

#include <cstring>
#include <filesystem>
#include <iostream>
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
		auto fileStream = std::make_unique<util::IFileStream>(file);
		char header[CHANNEL_FILE_HEADER.size() + 1];
		header[CHANNEL_FILE_HEADER.size()] = '\0';
		if (const uint32_t read = fileStream->read(header, CHANNEL_FILE_HEADER.size()); read != CHANNEL_FILE_HEADER.size() || header != CHANNEL_FILE_HEADER) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Invalid channel file for channel " << hash << std::endl;
			return;
		}
		uint32_t version;
		*fileStream >> version;
		if (version > CHANNEL_FILE_VERSION) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Unsupported channel file version for channel " << hash << std::endl;
			return;
		}
		if (version != CHANNEL_FILE_VERSION) {
			fileStream = nullptr;
			if (!migrateChannelFile(file)) {
				std::cerr << "Failed to migrate channel file for channel " << hash << std::endl;
				return;
			}
			fileStream = std::make_unique<util::IFileStream>(file);
			fileStream->read(nullptr, CHANNEL_FILE_HEADER.size() + 4);
		}
		if (version == 0) {
			*fileStream >> mName;
			uint32_t messageCount;
			*fileStream >> messageCount;
			mMessages.reserve(messageCount);
			for (uint32_t i = 0; i < messageCount; i++) {
				char userIDChars[20];
				fileStream->read(userIDChars, 20);
				user::manager::UserID fromID(userIDChars);
				std::string content;
				*fileStream >> content;
				const std::string& username = user::manager::loadUser(fromID).getUsername();
				mMessages.push_back({.content = content, .from = username, .fromID = fromID, .channelID = mID});
				user::manager::unloadUser(fromID);
			}
			uint32_t userCount;
			*fileStream >> userCount;
			for (uint32_t i = 0; i < userCount; i++) {
				char userIDChars[20];
				fileStream->read(userIDChars, 20);
				mUsers.emplace_back(userIDChars);
			}
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
		fileStream.write(CHANNEL_FILE_HEADER.c_str(), CHANNEL_FILE_HEADER.size());
		fileStream << CHANNEL_FILE_VERSION;
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

	bool migrateChannelFile(const std::string& file) {
		util::IFileStream fileStream(file);
		char header[CHANNEL_FILE_HEADER.size() + 1];
		header[CHANNEL_FILE_HEADER.size()] = '\0';
		if (const uint32_t read = fileStream.read(header, CHANNEL_FILE_HEADER.size()); read != CHANNEL_FILE_HEADER.size() || header != CHANNEL_FILE_HEADER) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Invalid channel file " << file << std::endl;
			return false;
		}
		uint32_t version;
		fileStream >> version;
		if (version > CHANNEL_FILE_VERSION) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Unsupported channel file " << file << std::endl;
			return false;
		}
		return false;
	}

	uint32_t migrateAllChannels() {
		uint32_t count = 0;
		for (const std::filesystem::directory_entry& channelFolder : std::filesystem::directory_iterator(CHANNELS_DIR)) {
			if (channelFolder.is_directory()) {
				for (const std::filesystem::directory_entry& channelFile : std::filesystem::directory_iterator(channelFolder.path())) {
					if (channelFile.is_regular_file()) {
						count += migrateChannelFile(channelFile.path().string());
					}
				}
			}
		}
		return count;
	}
}
