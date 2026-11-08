#include "User.h"

#include <cstring>
#include <random>
#include <utility>

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

	void User::setPasswordSalt(const char* salt) {
		memcpy(mPasswordSalt, salt, 16);
	}

	void User::setPasswordDigest(const char* digest) {
		memcpy(mPasswordDigest, digest, 20);
	}

	void User::setUsername(const std::string& username) {
		mUsername = username;
	}

	User::User(const manager::UserID& id) : mID(id) {}
}
