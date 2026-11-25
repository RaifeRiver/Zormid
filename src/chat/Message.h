#pragma once

#include <string>

#include "TextChannel.h"
#include "user/UserManager.h"

namespace zormid::chat {
	struct Message {
		std::string content;
		std::string from;
		user::manager::UserID fromID;
		ChannelID channelID;
		int64_t timestamp;
	};
}
