#include "User.h"

#include <cstring>
#include <random>

namespace zormid::user {
	Session::Session(const char* id) {
		memcpy(mID, id, 16);
	}

	const Session& User::startSession() {
		std::random_device randomDevice;
		char sessionID[16];
		for (char& c : sessionID) {
			c = static_cast<char>(randomDevice());
		}
		mSessions.emplace_back(sessionID);
		return mSessions.back();
	}

	User::User(const manager::UserID& id) : mID(id) {}
}
