#pragma once

#include <vector>

#include "UserManager.h"

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

		[[nodiscard]] char* getPasswordDigest() {
			return mPasswordDigest;
		}

		[[nodiscard]] std::vector<Session>& getSessions() {
			return mSessions;
		}

	private:
		manager::UserID mID;
		char mPasswordSalt[16] = {};
		char mPasswordDigest[20] = {};
		std::vector<Session> mSessions = {};

		explicit User(const manager::UserID& id);

		friend User manager::loadUser(const manager::UserID& id);
	};
}
