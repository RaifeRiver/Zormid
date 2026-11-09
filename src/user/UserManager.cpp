#include "UserManager.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <utility>

#include "TinySHA1.hpp"
#include "User.h"
#include "util/Base16.h"
#include "util/FileStream.h"

namespace zormid::user {
	manager::UserID::UserID(const char* id) {
		memcpy(mID, id, 20);
	}

	void manager::init() {
		if (!std::filesystem::exists(USERS_DIR)) {
			std::filesystem::create_directories(USERS_DIR);
		}
		else if (!std::filesystem::is_directory(USERS_DIR)) {
			std::filesystem::remove_all(USERS_DIR);
			std::filesystem::create_directories(USERS_DIR);
		}
	}

	manager::UserID manager::getUserID(const std::string& username) {
		sha1::SHA1 sha;
		sha.processBlock(username.c_str(), username.c_str() + username.length());
		uint8_t digest[20];
		sha.getDigestBytes(digest);
		return UserID(reinterpret_cast<char*>(digest));
	}

	bool manager::exists(const UserID& id) {
		const std::string hash = util::base16::encode(static_cast<const char*>(id), 20);
		const std::string dir = hash.substr(0, 2);
		if (!std::filesystem::exists(USERS_DIR + dir)) {
			return false;
		}
		const std::string file = hash.substr(2);
		return std::filesystem::exists(USERS_DIR + dir + "/" + file + ".dat");
	}

	User manager::create(const std::string& username, const std::string& password) {
		const UserID id = getUserID(username);
		const std::string hash = util::base16::encode(static_cast<const char*>(id), 20);
		if (const std::string dir = hash.substr(0, 2); !std::filesystem::exists(USERS_DIR + dir)) {
			std::filesystem::create_directories(USERS_DIR + dir);
		}
		const std::string file = hash.substr(2);
		std::string salt;
		std::random_device randomDevice;
		for (int i = 0; i < 16; i++) {
			salt += static_cast<char>(randomDevice());
		}
		const std::string passwordWithSalt = password + salt;
		sha1::SHA1 sha;
		sha.processBlock(passwordWithSalt.c_str(), passwordWithSalt.c_str() + passwordWithSalt.length());
		uint8_t digest[20];
		sha.getDigestBytes(digest);
		User user(id);
		user.setPasswordSalt(salt.data());
		user.setPasswordDigest(reinterpret_cast<char*>(digest));
		user.setUsername(username);
		saveUser(user);
		return user;
	}

	bool manager::verify(const UserID& id, const std::string& password) {
		const std::string hash = util::base16::encode(static_cast<const char*>(id), 20);
		const std::string dir = hash.substr(0, 2);
		if (!std::filesystem::exists(USERS_DIR + dir)) {
			return false;
		}
		const std::string file = USERS_DIR + dir + "/" + hash.substr(2) + ".dat";
		if (!std::filesystem::exists(file)) {
			return false;
		}
		util::IFileStream fileStream(file);
		char salt[17];
		fileStream.read(salt, sizeof(salt) - 1);
		uint8_t savedDigest[20];
		fileStream.read(savedDigest, sizeof(savedDigest));
		salt[16] = '\0';
		std::string passwordWithSalt = password + salt;
		sha1::SHA1 sha;
		sha.processBlock(passwordWithSalt.c_str(), passwordWithSalt.c_str() + passwordWithSalt.length());
		uint8_t digest[20];
		sha.getDigestBytes(digest);
		return std::memcmp(digest, savedDigest, sizeof(digest)) == 0;
	}

	User manager::loadUser(const UserID& id) {
		if (!exists(id)) {
			throw std::runtime_error("User does not exist");
		}
		User user(id);
		const std::string hash = util::base16::encode(static_cast<const char*>(id), 20);
		util::IFileStream fileStream(USERS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".dat");
		fileStream.read(user.getPasswordSalt(), 16);
		fileStream.read(user.getPasswordDigest(), 20);
		uint32_t sessionCount;
		fileStream >> sessionCount;
		for (uint32_t i = 0; i < sessionCount; i++) {
			char sessionID[16];
			fileStream.read(sessionID, sizeof(sessionID));
			user.getSessions().emplace_back(sessionID);
		}
		fileStream >> user.getUsername();
		uint32_t friendCount;
		fileStream >> friendCount;
		for (uint32_t i = 0; i < friendCount; i++) {
			char friendID[20];
			fileStream.read(friendID, sizeof(friendID));
			user.getFriends().emplace_back(friendID);
		}
		uint32_t channelCount;
		fileStream >> channelCount;
		for (uint32_t i = 0; i < channelCount; i++) {
			char channelID[20];
			fileStream.read(channelID, sizeof(channelID));
			user.getChannels().emplace_back(channelID);
		}
		return user;
	}

	void manager::saveUser(User& user) {
		const std::string hash = util::base16::encode(static_cast<const char*>(user.getID()), 20);
		util::OFileStream fileStream(USERS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".dat");
		fileStream.write(user.getPasswordSalt(), 16);
		fileStream.write(user.getPasswordDigest(), 20);
		fileStream << static_cast<uint32_t>(user.getSessions().size());
		for (const Session& session: user.getSessions()) {
			fileStream.write(static_cast<const char*>(session), 16);
		}
		fileStream << user.getUsername();
		fileStream << static_cast<uint32_t>(user.getFriends().size());
		for (const UserID& friendID: user.getFriends()) {
			fileStream.write(static_cast<const char*>(friendID), 20);
		}
		fileStream << static_cast<uint32_t>(user.getChannels().size());
		for (const chat::ChannelID& channelID: user.getChannels()) {
			fileStream.write(static_cast<const char*>(channelID), 20);
		}
	}
}
