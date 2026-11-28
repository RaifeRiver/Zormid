#include "Channel.h"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <random>

#include "TextChannel.h"
#include "VoiceChannel.h"
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

	void Channel::create() {
		if (mNeedsNewSave) {
			createNewSave_();
			mNeedsNewSave = false;
		}
	}

	void Channel::setName(const std::string& name) {
		mName = name;
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		util::OFileStream fileStream(CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".dat");
		fileStream.write(CHANNEL_FILE_HEADER.c_str(), CHANNEL_FILE_HEADER.size());
		fileStream << CHANNEL_FILE_VERSION;
		fileStream << static_cast<uint32_t>(mType);
		fileStream << mName;
	}

	void Channel::addUser(const user::manager::UserID& id) {
		mUsers.push_back(id);
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		util::OFileStream userFileStream(CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".users.dat", true);
		userFileStream << static_cast<uint32_t>(mUsers.size());
		userFileStream.seek(4 + (mUsers.size() - 1) * user::manager::USER_ID_SIZE);
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

	void Channel::save() {
		save_();
	}

	void Channel::deleteChannel() {
		mUsers.clear();
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		const std::string file = CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2);
		std::filesystem::remove(file + ".dat");
		std::filesystem::remove(file + ".users.dat");
		deleteChannel_();
	}

	Channel::Channel(const ChannelID& id, const ChannelType type) : mID(id), mType(type) {
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
		*fileStream >> reinterpret_cast<uint32_t&>(mType);
		if (mType != type) {
			throw std::runtime_error("Invalid channel type");
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
	}

	void Channel::load() {}

	void Channel::save_() {}

	void Channel::deleteChannel_() {}

	void Channel::createNewSave_() {}

	void Channel::createNewSave() {
		const std::string hash = util::base16::encode(static_cast<const char*>(mID), 20);
		std::filesystem::create_directories(CHANNELS_DIR + hash.substr(0, 2));
		const std::string file = CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2);
		util::OFileStream fileStream(file + ".dat");
		fileStream.write(CHANNEL_FILE_HEADER.c_str(), CHANNEL_FILE_HEADER.size());
		fileStream << CHANNEL_FILE_VERSION;
		fileStream << static_cast<uint32_t>(mType);
		fileStream << mName;
		util::OFileStream userFileStream(file + ".users.dat");
		userFileStream << 0u;
		mNeedsNewSave = true;
	}

	std::unique_ptr<Channel> loadChannel(ChannelID id) {
		const std::string hash = util::base16::encode(static_cast<const char*>(id), 20);
		const std::string dir = hash.substr(0, 2);
		if (!std::filesystem::exists(CHANNELS_DIR + dir)) {
			return nullptr;
		}
		const std::string file = CHANNELS_DIR + dir + "/" + hash.substr(2);
		if (!std::filesystem::exists(file + ".dat")) {
			return nullptr;
		}
		ChannelType type;
		{
			util::IFileStream fileStream(file + ".dat");
			fileStream.seek(CHANNEL_FILE_HEADER.size() + 4);
			fileStream >> reinterpret_cast<uint32_t&>(type);
		}
		switch (type) {
			case ChannelType::TEXT:
				return std::make_unique<TextChannel>(id);
			case ChannelType::VOICE:
				return std::make_unique<VoiceChannel>(id);
			default:
				return nullptr;
		}
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
		if (version < 3) {
			std::cerr << "Channel file too old " << file << std::endl;
			return false;
		}
		bool migrated = false;
		return migrated;
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

std::size_t std::hash<zormid::chat::ChannelID>::operator()(const zormid::chat::ChannelID& id) const noexcept {
	return std::hash<std::string_view>{}(std::string_view(static_cast<const char*>(id), 20));
}
