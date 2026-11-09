#pragma once

#include <vector>

#include "UserManager.h"
#include "chat/Channel.h"

namespace zormid::user {
	struct Session {
		explicit Session(const char* id);

		explicit operator const char*() const {
			return mID;
		}

	private:
		char mID[16] = {};
	};

	class User {
	public:
		const Session& startSession();

		[[nodiscard]] const manager::UserID& getID() const {
			return mID;
		}

		[[nodiscard]] char* getPasswordSalt() {
			return mPasswordSalt;
		}

		void setPasswordSalt(const char* salt);

		[[nodiscard]] char* getPasswordDigest() {
			return mPasswordDigest;
		}

		void setPasswordDigest(const char* digest);

		[[nodiscard]] std::vector<Session>& getSessions() {
			return mSessions;
		}

		[[nodiscard]] std::string& getUsername() {
			return mUsername;
		}

		void setUsername(const std::string& username);

		[[nodiscard]] std::vector<manager::UserID>& getFriends() {
			return mFriends;
		}

		[[nodiscard]] std::vector<chat::ChannelID>& getChannels() {
			return mChannels;
		}

	private:
		manager::UserID mID;
		char mPasswordSalt[16] = {};
		char mPasswordDigest[20] = {};
		std::string mUsername;
		std::vector<Session> mSessions = {};
		std::vector<manager::UserID> mFriends = {};
		std::vector<chat::ChannelID> mChannels = {};

		explicit User(const manager::UserID& id);

		friend User manager::create(const std::string&, const std::string&);
		friend User manager::loadUser(const manager::UserID&);
	};
}
