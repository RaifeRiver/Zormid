#include "CallManager.h"

#include <unordered_map>

namespace zormid::chat::call {
	static std::unordered_map<ChannelID, std::vector<client::Client*>> clientsInCalls;

	std::vector<client::Client*>& getClientsInCall(const ChannelID& channelID) {
		auto it = clientsInCalls.find(channelID);
		if (it == clientsInCalls.end()) {
			clientsInCalls[channelID] = {};
			it = clientsInCalls.find(channelID);
		}
		return it->second;
	}

	void leaveCall(const ChannelID& channelID, client::Client* client) {
		erase_if(clientsInCalls[channelID], [client](const client::Client* c) {
			return c == client;
		});
	}
}
