#pragma once

#include <memory>

#include "Config.h"
#include "client/Client.h"
#include "net/http/HTTPServer.h"

namespace zormid {
	static constexpr std::string ZORMID_VERSION = "v0.1";

	class Zormid {
	public:
		void run();

		void stop();

		[[nodiscard]] Config& getConfig() {
			return mConfig;
		}

		[[nodiscard]] std::vector<client::Client>& getClients() {
			return mClients;
		}

		static void init();

		static Zormid& getInstance() {
			return *sInstance;
		}

	private:
		static inline std::unique_ptr<Zormid> sInstance = nullptr;

		bool mRunning = true;
		Config mConfig;
		std::unique_ptr<net::http::HTTPServer> mServer;
		std::vector<client::Client> mClients;

		Zormid();
	};
}
