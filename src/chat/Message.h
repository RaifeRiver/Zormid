#pragma once

#include <string>

namespace zormid::chat {
	struct Message {
		std::string content;
		std::string from;
	};
}
