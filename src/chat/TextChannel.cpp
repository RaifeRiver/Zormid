#include "TextChannel.h"

#include <filesystem>
#include <iostream>

#include "Message.h"
#include "user/User.h"
#include "util/Base16.h"
#include "util/FileStream.h"

namespace zormid::chat {
	TextChannel::TextChannel(const ChannelID& id) : Channel(id, ChannelType::TEXT) {
		const std::string hash = util::base16::encode(static_cast<const char*>(id), 20);
		const std::string dir = hash.substr(0, 2);
		const std::string file = CHANNELS_DIR + dir + "/" + hash.substr(2);
		util::IFileStream messageInfoFileStream(file + ".message_info.dat");
		messageInfoFileStream >> mMessageCount;
	}

	uint32_t TextChannel::send(const Message& message) {
		uint32_t index = mMessageCount++;
		const std::string hash = util::base16::encode(static_cast<const char*>(getID()), 20);
		const std::string file = CHANNELS_DIR + hash.substr(0, 2) + "/" + hash.substr(2);
		util::OFileStream messageInfoFileStream(file + ".message_info.dat", true);
		messageInfoFileStream << mMessageCount;
		messageInfoFileStream.seekEnd();
		util::OFileStream messageFileStream(file + ".messages.dat", true);
		messageFileStream.seekEnd();
		messageInfoFileStream << message.fromID;
		messageInfoFileStream << messageFileStream.getPosition();
		messageInfoFileStream << message.timestamp;
		messageFileStream << message.content;
		return index;
	}

	std::vector<Message> TextChannel::getMessages(uint32_t startIndex, uint32_t count) const {
		const std::string hash = util::base16::encode(static_cast<const char*>(getID()), 20);
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
			int64_t timestamp;
			messageInfoFileStream >> timestamp;
			messageFileStream.seek(pos);
			std::string content;
			messageFileStream >> content;
			messages.push_back({.content = content, .from = user::manager::loadUser(fromID).getUsername(), .fromID = fromID, .channelID = getID(), .timestamp = timestamp});
			user::manager::unloadUser(fromID);
		}
		return messages;
	}

	void TextChannel::deleteChannel_() {
		mMessageCount = 0;
		const std::string hash = util::base16::encode(static_cast<const char*>(getID()), 20);
		const std::string dir = hash.substr(0, 2);
		const std::string file = CHANNELS_DIR + dir + "/" + hash.substr(2);
		std::filesystem::remove(file + ".message_info.dat");
		std::filesystem::remove(file + ".messages.dat");
	}

	void TextChannel::createNewSave_() {std::cout << "Create new save" << std::endl;
		const std::string hash = util::base16::encode(static_cast<const char*>(getID()), 20);
		const std::string dir = hash.substr(0, 2);
		const std::string file = CHANNELS_DIR + dir + "/" + hash.substr(2);
		util::OFileStream messageInfoFileStream(file + ".message_info.dat");
		messageInfoFileStream << 0u;
		util::OFileStream messageFileStream(file + ".messages.dat");
	}
}
