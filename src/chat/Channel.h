#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "user/UserManager.h"

namespace zormid::chat {
	struct Message;

	const std::string CHANNELS_DIR = "../data/channels/";
	constexpr std::string CHANNEL_FILE_HEADER = "ZORMCHAN";
	constexpr uint32_t CHANNEL_FILE_VERSION = 0;

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

		void send(const Message& message);

		[[nodiscard]] std::vector<Message>& getMessages() {
			return mMessages;
		}

		[[nodiscard]] std::vector<user::manager::UserID>& getUsers() {
			return mUsers;
		}

		[[nodiscard]] std::string& getName() {
			return mName;
		}

		void setName(const std::string& name);

		void save() const;

	private:
		ChannelID mID;
		std::string mName;
		std::vector<Message> mMessages;
		std::vector<user::manager::UserID> mUsers;
	};
}
