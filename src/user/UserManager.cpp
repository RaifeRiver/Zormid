#include "UserManager.h"

#include <filesystem>
#include <fstream>
#include <random>

#include "TinySHA1.hpp"
#include "util/Base16.h"

namespace zormid::user {
	void manager::init() {
		if (!std::filesystem::exists(USERS_DIR)) {
			std::filesystem::create_directories(USERS_DIR);
		}
		else if (!std::filesystem::is_directory(USERS_DIR)) {
			std::filesystem::remove_all(USERS_DIR);
			std::filesystem::create_directories(USERS_DIR);
		}
	}

	static std::string generateHash(const std::string& username) {
		sha1::SHA1 sha;
		sha.processBlock(username.c_str(), username.c_str() + username.length());
		uint8_t digest[20];
		sha.getDigestBytes(digest);
		return util::base16::encode(digest, sizeof(digest));
	}

	bool manager::exists(const std::string& username) {
		const std::string hash = generateHash(username);
		const std::string dir = hash.substr(0, 2);
		if (!std::filesystem::exists(USERS_DIR + dir)) {
			return false;
		}
		const std::string file = hash.substr(2);
		return std::filesystem::exists(USERS_DIR + dir + "/" + file + ".dat");
	}

	void manager::create(const std::string& username, const std::string& password) {
		const std::string hash = generateHash(username);
		const std::string dir = hash.substr(0, 2);
		if (!std::filesystem::exists(USERS_DIR + dir)) {
			std::filesystem::create_directories(USERS_DIR + dir);
		}
		const std::string file = hash.substr(2);
		std::string salt;
		std::random_device randomDevice;
		for (int i = 0; i < 16; i++) {
			salt += static_cast<char>(randomDevice());
		}
		std::string passwordWithSalt = password + salt;
		sha1::SHA1 sha;
		sha.processBlock(passwordWithSalt.c_str(), passwordWithSalt.c_str() + passwordWithSalt.length());
		uint8_t digest[20];
		sha.getDigestBytes(digest);
		std::ofstream fileStream(USERS_DIR + dir + "/" + file + ".dat", std::ios::binary);
		fileStream.write(reinterpret_cast<char*>(digest), sizeof(digest));
		fileStream.write(salt.c_str(), static_cast<ssize_t>(salt.length()));
		fileStream.close();
	}

	bool manager::verify(const std::string& username, const std::string& password) {
		const std::string hash = generateHash(username);
		const std::string dir = hash.substr(0, 2);
		if (!std::filesystem::exists(USERS_DIR + dir)) {
			return false;
		}
		const std::string file = USERS_DIR + dir + "/" + hash.substr(2) + ".dat";
		if (!std::filesystem::exists(file)) {
			return false;
		}
		std::ifstream fileStream(file);
		uint8_t savedDigest[20];
		fileStream.read(reinterpret_cast<char*>(savedDigest), sizeof(savedDigest));
		char salt[16];
		fileStream.read(salt, sizeof(salt));
		std::string passwordWithSalt = password + salt;
		sha1::SHA1 sha;
		sha.processBlock(passwordWithSalt.c_str(), passwordWithSalt.c_str() + passwordWithSalt.length());
		uint8_t digest[20];
		sha.getDigestBytes(digest);
		return std::memcmp(digest, savedDigest, sizeof(digest)) == 0;
	}
}
