#include "UserManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <utility>

#include "TinySHA1.hpp"
#include "User.h"
#include "util/Base64.h"
#include "util/FileStream.h"

namespace zormid::user::manager {
	UserID::UserID() {
		std::string hash;
		do {
			std::random_device randomDevice;
			for (char& c : mID) {
				c = static_cast<char>(randomDevice());
			}
			hash = util::base64::encodeURL(mID, USER_ID_SIZE);
		}
		while (std::filesystem::exists(USERS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".dat"));
	}

	UserID::UserID(const char* id) {
		memcpy(mID, id, USER_ID_SIZE);
	}

	void init() {
		if (!std::filesystem::exists(USERS_DIR)) {
			std::filesystem::create_directories(USERS_DIR);
		}
		else if (!std::filesystem::is_directory(USERS_DIR)) {
			std::filesystem::remove_all(USERS_DIR);
			std::filesystem::create_directories(USERS_DIR);
		}
	}

	std::optional<UserID> getUserID(const std::string& username) {
		sha1::SHA1 usernameSHA;
		usernameSHA.processBlock(username.c_str(), username.c_str() + username.length());
		uint8_t usernameDigest[20];
		usernameSHA.getDigestBytes(usernameDigest);
		const std::string usernameHash = util::base64::encodeURL(usernameDigest, 20);
		util::IFileStream file(USERS_DIR + usernameHash.substr(0, 2) + "/" + usernameHash.substr(2) + ".dat");
		if (!file.isOpen()) {
			return {};
		}
		return file.readUserID();
	}

	static std::unordered_map<UserID, User> users;

	bool exists(const UserID& id) {
		if (users.contains(id)) {
			return true;
		}
		const std::string hash = util::base64::encodeURL(static_cast<const char*>(id), USER_ID_SIZE);
		const std::string dir = hash.substr(0, 2);
		if (!std::filesystem::exists(USERS_DIR + dir)) {
			return false;
		}
		const std::string file = hash.substr(2);
		return std::filesystem::exists(USERS_DIR + dir + "/" + file + ".dat");
	}

	User& create(const std::string& username, const std::string& password) {
		const UserID id;
		sha1::SHA1 usernameSHA;
		usernameSHA.processBlock(username.c_str(), username.c_str() + username.length());
		uint8_t usernameDigest[20];
		usernameSHA.getDigestBytes(usernameDigest);
		const std::string usernameHash = util::base64::encodeURL(usernameDigest, 20);
		const std::string dir = USERS_DIR + usernameHash.substr(0, 2);
		if (!std::filesystem::exists(dir)) {
			std::filesystem::create_directories(dir);
		}
		util::OFileStream usernameFile(dir + "/" + usernameHash.substr(2) + ".dat");
		usernameFile.write(static_cast<const char*>(id), USER_ID_SIZE);
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
		user.setActiveClients(1);
		users.emplace(id, user);
		return users.at(id);
	}

	bool verify(const UserID& id, const std::string& password) {
		if (const auto it = users.find(id); it != users.end()) {
			std::string passwordWithSalt = password + std::string(it->second.getPasswordSalt(), 16);
			sha1::SHA1 sha;
			sha.processBlock(passwordWithSalt.c_str(), passwordWithSalt.c_str() + passwordWithSalt.length());
			uint8_t digest[20];
			sha.getDigestBytes(digest);
			return std::memcmp(digest, it->second.getPasswordDigest(), sizeof(digest)) == 0;
		}
		const std::string hash = util::base64::encodeURL(static_cast<const char*>(id), USER_ID_SIZE);
		const std::string dir = hash.substr(0, 2);
		if (!std::filesystem::exists(USERS_DIR + dir)) {
			return false;
		}
		const std::string file = USERS_DIR + dir + "/" + hash.substr(2) + ".dat";
		if (!std::filesystem::exists(file)) {
			return false;
		}
		util::IFileStream fileStream(file);
		char header[USER_FILE_HEADER.size() + 1];
		header[USER_FILE_HEADER.size()] = '\0';
		if (uint32_t read = fileStream.read(header, USER_FILE_HEADER.size()); read != USER_FILE_HEADER.size() || header != USER_FILE_HEADER) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Invalid user file for user " << hash << std::endl;
			return false;
		}
		uint32_t version;
		fileStream >> version;
		if (version > USER_FILE_VERSION) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Unsupported user file version for user " << hash << std::endl;
			return false;
		}
		char salt[17];
		fileStream.read(salt, sizeof(salt) - 1);
		uint8_t savedDigest[20];
		fileStream.read(savedDigest, sizeof(savedDigest));
		salt[16] = '\0';
		std::string passwordWithSalt = password + std::string(salt, 16);
		sha1::SHA1 sha;
		sha.processBlock(passwordWithSalt.c_str(), passwordWithSalt.c_str() + passwordWithSalt.length());
		uint8_t digest[20];
		sha.getDigestBytes(digest);
		return std::memcmp(digest, savedDigest, sizeof(digest)) == 0;
	}

	User& loadUser(const UserID& id) {
		if (!exists(id)) {
			throw std::runtime_error("User does not exist");
		}
		if (const auto it = users.find(id); it != users.end()) {
			User& user = it->second;
			user.getActiveClients()++;
			return user;
		}
		User user(id);
		const std::string hash = util::base64::encodeURL(static_cast<const char*>(id), USER_ID_SIZE);
		const std::string file = USERS_DIR + hash.substr(0, 2) + "/" + hash.substr(2) + ".dat";
		auto fileStream = std::make_unique<util::IFileStream>(file);
		char header[USER_FILE_HEADER.size() + 1];
		header[USER_FILE_HEADER.size()] = '\0';
		if (const uint32_t read = fileStream->read(header, USER_FILE_HEADER.size()); read != USER_FILE_HEADER.size() || header != USER_FILE_HEADER) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Invalid user file for user " << hash << std::endl;
			throw std::runtime_error("Invalid user file");
		}
		uint32_t version;
		*fileStream >> version;
		if (version > USER_FILE_VERSION) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Unsupported user file version for user " << hash << std::endl;
			throw std::runtime_error("Unsupported user file version");
		}
		if (version < 1) {
			fileStream = nullptr;
			if (!migrateUserFile(file)) {
				std::cerr << "Failed to migrate user file for user " << hash << std::endl;
				throw std::runtime_error("Failed to migrate user file");
			}
			fileStream = std::make_unique<util::IFileStream>(file);
			fileStream->read(nullptr, USER_FILE_HEADER.size() + 4);
		}
		fileStream->read(user.getPasswordSalt(), 16);
		fileStream->read(user.getPasswordDigest(), 20);
		uint32_t sessionCount;
		*fileStream >> sessionCount;
		if (version == 1) {
			fileStream->read(nullptr, sessionCount * 16);
		}
		else {
			for (uint32_t i = 0; i < sessionCount; i++) {
				char sessionID[16];
				fileStream->read(sessionID, sizeof(sessionID));
				std::string userAgent;
				*fileStream >> userAgent;
				user.getSessions().emplace_back(sessionID, userAgent);
			}
		}
		*fileStream >> user.getUsername();
		uint32_t friendCount;
		*fileStream >> friendCount;
		for (uint32_t i = 0; i < friendCount; i++) {
			char friendID[USER_ID_SIZE];
			fileStream->read(friendID, USER_ID_SIZE);
			user.getFriends().emplace_back(friendID);
		}
		uint32_t channelCount;
		*fileStream >> channelCount;
		for (uint32_t i = 0; i < channelCount; i++) {
			char channelID[20];
			fileStream->read(channelID, sizeof(channelID));
			user.getChannels().emplace_back(channelID);
		}
		user.setActiveClients(1);
		users.emplace(id, user);
		return users.at(id);
	}

	void unloadUser(const UserID& id) {
		const auto it = users.find(id);
		if (it == users.end()) {
			return;
		}
		User& user = it->second;
		if (--user.getActiveClients() > 0) {
			return;
		}
		const std::string hash = util::base64::encodeURL(static_cast<const char*>(id), USER_ID_SIZE);
		const std::string dir = USERS_DIR + hash.substr(0, 2);
		if (!std::filesystem::exists(dir)) {
			std::filesystem::create_directory(dir);
		}
		util::OFileStream fileStream(dir + "/" + hash.substr(2) + ".dat");
		fileStream.write(USER_FILE_HEADER.c_str(), USER_FILE_HEADER.size());
		fileStream << USER_FILE_VERSION;
		fileStream.write(user.getPasswordSalt(), 16);
		fileStream.write(user.getPasswordDigest(), 20);
		fileStream << static_cast<uint32_t>(user.getSessions().size());
		for (const Session& session: user.getSessions()) {
			fileStream.write(static_cast<const char*>(session), 16);
			fileStream << session.getUserAgent();
		}
		fileStream << user.getUsername();
		fileStream << static_cast<uint32_t>(user.getFriends().size());
		for (const UserID& friendID: user.getFriends()) {
			fileStream << friendID;
		}
		fileStream << static_cast<uint32_t>(user.getChannels().size());
		for (const chat::ChannelID& channelID: user.getChannels()) {
			fileStream.write(static_cast<const char*>(channelID), 20);
		}
		users.erase(id);
	}

	std::unordered_map<UserID, User>& getLoadedUsers() {
		return users;
	}

	bool isActive(const UserID& userId) {
		return users.contains(userId);
	}

	void unloadAllUsers() {
		while (!users.empty()) {
			users.begin()->second.setActiveClients(1);
			unloadUser(users.begin()->first);
		}
	}

	bool migrateUserFile(const std::string& file) {
		util::IFileStream fileStream(file);
		char header[USER_FILE_HEADER.size() + 1];
		header[USER_FILE_HEADER.size()] = '\0';
		if (const uint32_t read = fileStream.read(header, USER_FILE_HEADER.size()); read != USER_FILE_HEADER.size() || header != USER_FILE_HEADER) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Invalid user file " << file << std::endl;
			return false;
		}
		uint32_t version;
		fileStream >> version;
		if (version > USER_FILE_VERSION) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Unsupported user file " << file << std::endl;
			return false;
		}
		if (version == 0) {
			int32_t attempt = -1;
			while (std::filesystem::exists(file + "." + std::to_string(++attempt))) {}
			std::filesystem::rename(file, file + "." + std::to_string(attempt));
			std::cerr << "Unsupported user file " << file << std::endl;
			return false;
		}
		return false;
	}

	uint32_t migrateAllUsers() {
		uint32_t count = 0;
		for (const std::filesystem::directory_entry& userFolder : std::filesystem::directory_iterator(USERS_DIR)) {
			if (userFolder.is_directory()) {
				for (const std::filesystem::directory_entry& userFile : std::filesystem::directory_iterator(userFolder.path())) {
					if (userFile.is_regular_file()) {
						count += migrateUserFile(userFile.path().string());
					}
				}
			}
		}
		return count;
	}

	void changeUsername(const UserID& id, const std::string& username) {
		std::string oldUsername;
		if (const auto it = users.find(id); it != users.end()) {
			oldUsername = it->second.getUsername();
			it->second.setUsername(username);
		}
		else {
			oldUsername = loadUser(id).getUsername();
			unloadUser(id);
		}
		if (username == oldUsername) {
			return;
		}
		sha1::SHA1 sha;
		sha.processBlock(oldUsername.c_str(), oldUsername.c_str() + oldUsername.length());
		uint8_t oldDigest[20];
		sha.getDigestBytes(oldDigest);
		const std::string oldHash = util::base64::encodeURL(oldDigest, 20);
		sha.reset();
		sha.processBlock(username.c_str(), username.c_str() + username.length());
		uint8_t usernameDigest[20];
		sha.getDigestBytes(usernameDigest);
		const std::string usernameHash = util::base64::encodeURL(usernameDigest, 20);
		const std::string dir = USERS_DIR + usernameHash.substr(0, 2);
		if (!std::filesystem::exists(dir)) {
			std::filesystem::create_directories(dir);
		}
		std::filesystem::rename(USERS_DIR + oldHash.substr(0, 2) + "/" + oldHash.substr(2) + ".dat", dir + "/" + usernameHash.substr(2) + ".dat");
	}
}

std::size_t std::hash<zormid::user::manager::UserID>::operator()(const zormid::user::manager::UserID& id) const noexcept {
	return std::hash<std::string_view>{}(std::string_view(static_cast<const char*>(id), zormid::user::manager::USER_ID_SIZE));
}
