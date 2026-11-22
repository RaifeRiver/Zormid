#include "User.h"

#include <cstring>
#include <random>
#include <utility>

namespace zormid::user {
	Session::Session(const char* id, std::string userAgent) : mUserAgent(std::move(userAgent)) {
		memcpy(mID, id, 16);
	}

	void Session::setUserAgent(const std::string& userAgent) {
		mUserAgent = userAgent;
	}

	User::User(const manager::UserID& id) : mID(id) {}

	const Session& User::startSession(const std::string& userAgent) {
		std::random_device randomDevice;
		char sessionID[16];
		for (char& c : sessionID) {
			c = static_cast<char>(randomDevice());
		}
		mSessions.emplace_back(sessionID, userAgent);
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

	void User::setAvatar(const std::string& avatar) {
		mAvatar = avatar;
	}

	void User::setActiveClients(const uint32_t clients) {
		mActiveClients = clients;
	}

	void User::pushPayload(const net::http::PayloadData& payload) {
		mWriteQueue.push_back(payload);
	}

	void User::clearWriteQueue() {
		for (net::http::PayloadData& payload : mWriteQueue) {
			if (payload.data) {
				free(payload.data);
				payload.data = nullptr;
			}
		}
		mWriteQueue.clear();
	}
}
