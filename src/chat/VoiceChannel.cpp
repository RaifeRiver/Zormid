#include "VoiceChannel.h"

namespace zormid::chat {
	VoiceChannel::VoiceChannel(const ChannelID& id) : Channel(id, ChannelType::VOICE) {}
}
