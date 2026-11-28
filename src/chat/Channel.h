#pragma once

#include <memory>
#include <vector>

#include "user/UserManager.h"

namespace zormid::chat {
	const std::string CHANNELS_DIR = "../data/channels/";
	constexpr std::string CHANNEL_FILE_HEADER = "ZORMCHAN";
	constexpr uint32_t CHANNEL_FILE_VERSION = 3;

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

	enum class ChannelType : uint32_t {
		TEXT = 0,
		VOICE = 1
	};

	class Channel {
	public:
		void create();

		[[nodiscard]] ChannelID getID() const {
			return mID;
		}

		[[nodiscard]] ChannelType getType() const {
			return mType;
		}

		[[nodiscard]] std::string& getName() {
			return mName;
		}

		void setName(const std::string& name);

		void addUser(const user::manager::UserID& id);

		void removeUser(const user::manager::UserID& id);

		[[nodiscard]] const std::vector<user::manager::UserID>& getUsers() const {
			return mUsers;
		}

		void save();

		void deleteChannel();

		virtual ~Channel() = default;

	protected:
		explicit Channel(const ChannelID& id, ChannelType type);

		virtual void load();

		virtual void save_();

		virtual void deleteChannel_();

		virtual void createNewSave_();

	private:
		bool mNeedsNewSave = false;
		ChannelID mID;
		ChannelType mType;
		std::string mName;
		std::vector<user::manager::UserID> mUsers;

		void createNewSave();
	};

	std::unique_ptr<Channel> loadChannel(ChannelID id);

	bool migrateChannelFile(const std::string& file);

	uint32_t migrateAllChannels();
}

template <> struct std::hash<zormid::chat::ChannelID> {
	size_t operator()(const zormid::chat::ChannelID& id) const noexcept;
};