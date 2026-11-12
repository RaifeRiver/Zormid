#pragma once

#include <string>
#include <unordered_map>

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

		bool operator==(const UserID& other) const = default;

	private:
		char mID[20] = {};
	};

	void init();

	UserID getUserID(const std::string& username);

	bool exists(const UserID& id);

	User& create(const std::string& username, const std::string& password);

	bool verify(const UserID& id, const std::string& password);

	User& loadUser(const UserID& id);

	void unloadUser(const UserID& id);

	std::unordered_map<UserID, User>& getLoadedUsers();
}

template <> struct std::hash<zormid::user::manager::UserID> {
	size_t operator()(const zormid::user::manager::UserID& id) const noexcept;
};