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
			createNewSave();
			return;
		}
		const std::string file = CHANNELS_DIR + dir + "/" + hash.substr(2);
		if (!std::filesystem::exists(file + ".dat")) {
			createNewSave();
			return;
		}
		auto fileStream = std::make_unique<util::IFileStream>(file + ".dat");
		char header[CHANNEL_FILE_HEADER.size() + 1];
		header[CHANNEL_FILE_HEADER.size()] = '\0';
		if (const uint32_t read = fileStream->read(header, CHANNEL_FILE_HEADER.size()); read != CHANNEL_FILE_HEADER.size() || header != CHANNEL_FILE_HEADER) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + ".dat." + std::to_string(++attempt))) {}
			std::filesystem::rename(file + ".dat", file + ".dat." + std::to_string(attempt));
			std::cerr << "Invalid channel file for channel " << hash << std::endl;
			createNewSave();
			return;
		}
		uint32_t version;
		*fileStream >> version;
		if (version > CHANNEL_FILE_VERSION) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + ".dat." + std::to_string(++attempt))) {}
			std::filesystem::rename(file + ".dat", file + ".dat." + std::to_string(attempt));
			std::cerr << "Unsupported channel file version for channel " << hash << std::endl;
			createNewSave();
			return;
		}
		if (version != CHANNEL_FILE_VERSION) {
			fileStream = nullptr;
			if (!migrateChannelFile(file)) {
				std::cerr << "Failed to migrate channel file for channel " << hash << std::endl;
				createNewSave();
				return;
			}
			fileStream = std::make_unique<util::IFileStream>(file + ".dat");
			fileStream->read(nullptr, CHANNEL_FILE_HEADER.size() + 4);
		}
		*fileStream >> mName;
		util::IFileStream userFileStream(file + ".users.dat");
		uint32_t userCount;
		userFileStream >> userCount;
		mUsers.reserve(userCount);
		for (uint32_t i = 0; i < userCount; i++) {
			char userIDChars[user::manager::USER_ID_SIZE];
			userFileStream.read(userIDChars, user::manager::USER_ID_SIZE);
			mUsers.emplace_back(userIDChars);
		}
		util::IFileStream messageInfoFileStream(file + ".message_info.dat");
		messageInfoFileStream >> mMessageCount;
	}

	uint32_t Channel::send(const Message& message) {
		uint32_t index = mMessageCount++;
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		const std::string file = CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2);
		util::OFileStream messageInfoFileStream(file + ".message_info.dat", true);
		messageInfoFileStream << mMessageCount;
		messageInfoFileStream.seekEnd();
		util::OFileStream messageFileStream(file + ".messages.dat", true);
		messageFileStream.seekEnd();
		messageInfoFileStream << message.fromID;
		messageInfoFileStream << messageFileStream.getPosition();
		messageFileStream << message.content;
		return index;
	}

	void Channel::addUser(const user::manager::UserID& id) {
		mUsers.push_back(id);
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		util::OFileStream userFileStream(CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".users.dat", true);
		userFileStream << static_cast<uint32_t>(mUsers.size());
		userFileStream.seekEnd();
		userFileStream << id;
	}

	void Channel::removeUser(const user::manager::UserID& id) {
		const size_t index = std::ranges::find(mUsers.begin(), mUsers.end(), id) - mUsers.begin();
		const bool last = index == mUsers.size() - 1;
		if (!last) {
			mUsers[index] = mUsers.back();
		}
		mUsers.pop_back();
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		util::OFileStream userFileStream(CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".users.dat", true);
		userFileStream << static_cast<uint32_t>(mUsers.size());
		if (!last) {
			userFileStream.seek(index * user::manager::USER_ID_SIZE + 4);
			userFileStream << mUsers[index];
		}
	}

	std::vector<Message> Channel::getMessages(uint32_t startIndex, uint32_t count) const {
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		const std::string file = CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2);
		util::IFileStream messageInfoFileStream(file + ".message_info.dat");
		util::IFileStream messageFileStream(file + ".messages.dat");
		messageInfoFileStream.seek(startIndex * (4 + user::manager::USER_ID_SIZE) + 4);
		std::vector<Message> messages;
		messages.reserve(count);
		for (uint32_t i = 0; i < count; i++) {
			user::manager::UserID fromID = messageInfoFileStream.readUserID();
			uint32_t pos;
			messageInfoFileStream >> pos;
			messageFileStream.seek(pos);
			std::string content;
			messageFileStream >> content;
			messages.push_back({.content = content, .from = user::manager::loadUser(fromID).getUsername(), .fromID = fromID, .channelID = mID});
			user::manager::unloadUser(fromID);
		}
		return messages;
	}

	void Channel::setName(const std::string& name) {
		mName = name;
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		util::OFileStream fileStream(CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".dat");
		fileStream.write(CHANNEL_FILE_HEADER.c_str(), CHANNEL_FILE_HEADER.size());
		fileStream << CHANNEL_FILE_VERSION;
		fileStream << mName;
	}

	void Channel::deleteChannel() {
		mUsers.clear();
		mMessageCount = 0;
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		const std::string file = CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2);
		std::filesystem::remove(file + ".dat");
		std::filesystem::remove(file + ".users.dat");
		std::filesystem::remove(file + ".message_info.dat");
		std::filesystem::remove(file + ".messages.dat");
	}

	void Channel::createNewSave() const {
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		std::filesystem::create_directories(CHANNELS_DIR + hash.substr(0, 2));
		const std::string file = CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2);
		util::OFileStream fileStream(file + ".dat");
		fileStream.write(CHANNEL_FILE_HEADER.c_str(), CHANNEL_FILE_HEADER.size());
		fileStream << CHANNEL_FILE_VERSION;
		fileStream << mName;
		util::OFileStream userFileStream(file + ".users.dat");
		userFileStream << 0u;
		util::OFileStream messageInfoFileStream(file + ".message_info.dat");
		messageInfoFileStream << 0u;
		util::OFileStream messageFileStream(file + ".messages.dat");
	}

	bool migrateChannelFile(const std::string& file) {
		util::IFileStream fileStream(file + ".dat");
		char header[CHANNEL_FILE_HEADER.size() + 1];
		header[CHANNEL_FILE_HEADER.size()] = '\0';
		if (const uint32_t read = fileStream.read(header, CHANNEL_FILE_HEADER.size()); read != CHANNEL_FILE_HEADER.size() || header != CHANNEL_FILE_HEADER) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + ".dat." + std::to_string(++attempt))) {}
			std::filesystem::rename(file + ".dat", file + ".dat." + std::to_string(attempt));
			std::cerr << "Invalid channel file " << file << std::endl;
			return false;
		}
		uint32_t version;
		fileStream >> version;
		if (version > CHANNEL_FILE_VERSION) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + ".dat." + std::to_string(++attempt))) {}
			std::filesystem::rename(file + ".dat", file + ".dat." + std::to_string(attempt));
			std::cerr << "Unsupported channel file " << file << std::endl;
			return false;
		}
		if (version == 0) {
			std::string name;
			fileStream >> name;
			uint32_t messageCount;
			fileStream >> messageCount;
			std::vector<std::pair<user::manager::UserID, std::string>> messages;
			messages.reserve(messageCount);
			for (uint32_t i = 0; i < messageCount; i++) {
				char userIDChars[20];
				fileStream.read(userIDChars, 20);
				user::manager::UserID fromID(userIDChars);
				std::string content;
				fileStream >> content;
				messages.emplace_back(fromID, content);
			}
			uint32_t userCount;
			fileStream >> userCount;
			std::vector<user::manager::UserID> users;
			users.reserve(userCount);
			for (uint32_t i = 0; i < userCount; i++) {
				char userIDChars[20];
				fileStream.read(userIDChars, 20);
				users.emplace_back(userIDChars);
			}
			util::OFileStream outStream(file + ".dat");
			outStream.write(CHANNEL_FILE_HEADER.c_str(), CHANNEL_FILE_HEADER.size());
			outStream << 1u;
			outStream << name;
			util::OFileStream userFileStream(file + ".users.dat");
			userFileStream << userCount;
			for (const user::manager::UserID& userID: users) {
				userFileStream << userID;
			}
			util::OFileStream messageInfoFileStream(file + ".message_info.dat");
			messageInfoFileStream << messageCount;
			util::OFileStream messageFileStream(file + ".messages.dat");
			for (const auto& [userID, content] : messages) {
				messageInfoFileStream << userID;
				messageInfoFileStream << messageFileStream.getPosition();
				messageFileStream << content;
			}
			version = 1;
			return true;
		}
		return false;
	}

	uint32_t migrateAllChannels() {
		uint32_t count = 0;
		for (const std::filesystem::directory_entry& channelFolder : std::filesystem::directory_iterator(CHANNELS_DIR)) {
			if (channelFolder.is_directory()) {
				for (const std::filesystem::directory_entry& channelFile : std::filesystem::directory_iterator(channelFolder.path())) {
					if (std::string path = channelFile.path().string(); channelFile.is_regular_file() && path.ends_with(".dat")) {
						count += migrateChannelFile(path.substr(0, path.size() - 4));
					}
				}
			}
		}
		return count;
	}
}
