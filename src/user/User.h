#pragma once

#include <cstdint>
#include <vector>

#include "UserManager.h"
#include "chat/Channel.h"
#include "net/http/WebSocket.h"

namespace zormid::user {
	constexpr std::string USER_FILE_HEADER = "ZORMUSER";
	constexpr uint32_t USER_FILE_VERSION = 2;

	struct Session {
		explicit Session(const char* id, std::string userAgent);

		explicit operator const char*() const {
			return mID;
		}

		[[nodiscard]] std::string getUserAgent() const {
			return mUserAgent;
		}

		void setUserAgent(const std::string& userAgent);

	private:
		char mID[16] = {};
		std::string mUserAgent;
	};

	class User {
	public:
		explicit User(const manager::UserID& id);

		const Session& startSession(const std::string& userAgent);

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

		[[nodiscard]] uint32_t& getActiveClients() {
			return mActiveClients;
		}

		void setActiveClients(uint32_t clients);

		[[nodiscard]] std::vector<net::http::PayloadData>& getWriteQueue() {
			return mWriteQueue;
		}

		void pushPayload(const net::http::PayloadData& payload);

		void clearWriteQueue();

	private:
		manager::UserID mID;
		char mPasswordSalt[16] = {};
		char mPasswordDigest[20] = {};
		std::string mUsername;
		std::vector<Session> mSessions = {};
		std::vector<manager::UserID> mFriends = {};
		std::vector<chat::ChannelID> mChannels = {};
		uint32_t mActiveClients = 0;
		std::vector<net::http::PayloadData> mWriteQueue = {};
	};
}
