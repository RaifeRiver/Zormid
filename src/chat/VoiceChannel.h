#pragma once

#include "Channel.h"

namespace zormid::chat {
	class VoiceChannel : public Channel {
	public:
		explicit VoiceChannel(const ChannelID& id);
	};
}
