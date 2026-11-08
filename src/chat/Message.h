#pragma once

#include <string>

#include "user/UserManager.h"

namespace zormid::chat {
	struct Message {
		std::string content;
		std::string from;
		user::manager::UserID fromID;
	};
}
