#pragma once

#include <string>
#include <bits/hashtable_policy.h>

namespace zormid::user {
	class User;
}

namespace zormid::user::manager {
	constexpr std::string USERS_DIR = "../data/users/";

	struct UserID {
		explicit UserID(const char* id);

		explicit operator const char*() const {
			return mID;
		}

	private:
		char mID[20] = {};
	};

	void init();

	UserID getUserID(const std::string& username);

	bool exists(const UserID& id);

	void create(const UserID& id, const std::string& password);

	bool verify(const UserID& id, const std::string& password);

	User loadUser(const UserID& id);

	void saveUser(User& user);
}
