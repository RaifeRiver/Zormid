#pragma once

#include <string>

namespace zormid::user::manager {
	constexpr std::string USERS_DIR = "../data/users/";

	void init();

	bool exists(const std::string& username);

	void create(const std::string& username, const std::string& password);

	bool verify(const std::string& username, const std::string& password);
}
