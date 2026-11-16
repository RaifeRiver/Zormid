#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "user/UserManager.h"

namespace zormid::chat {
	struct Message;

	const std::string CHANNELS_DIR = "../data/channels/";
	constexpr std::string CHANNEL_FILE_HEADER = "ZORMCHAN";
	constexpr uint32_t CHANNEL_FILE_VERSION = 1;

	struct ChannelID {
		ChannelID();

		explicit ChannelID(const char* id);

		explicit operator const char*() const {
			return mID;
		}

		bool operator==(const ChannelID& other) const = default;

	private:
		char mID[20] = {};
	};

	class Channel {
	public:
		explicit Channel(const ChannelID& id);

		uint32_t send(const Message& message);

		void addUser(const user::manager::UserID& id);

		void removeUser(const user::manager::UserID& id);

		[[nodiscard]] const std::vector<user::manager::UserID>& getUsers() const {
			return mUsers;
		}

		[[nodiscard]] std::string& getName() {
			return mName;
		}

		[[nodiscard]] uint32_t getMessageCount() const {
			return mMessageCount;
		}

		[[nodiscard]] std::vector<Message> getMessages(uint32_t startIndex, uint32_t count) const;

		void setName(const std::string& name);

	private:
		ChannelID mID;
		std::string mName;
		std::vector<user::manager::UserID> mUsers;
		uint32_t mMessageCount = 0;

		void createNewSave() const;
	};

	bool migrateChannelFile(const std::string& file);

	uint32_t migrateAllChannels();
}
