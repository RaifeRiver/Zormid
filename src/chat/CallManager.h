#pragma once

#include <vector>

#include "Channel.h"

namespace zormid::client {
	class Client;
}

namespace zormid::chat::call {
	std::vector<client::Client*>& getClientsInCall(const ChannelID& channelID);

	void leaveCall(const ChannelID& channelID, client::Client* client);
}
