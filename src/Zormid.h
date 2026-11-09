#pragma once

#include <memory>

#include "Config.h"
#include "client/Client.h"
#include "net/http/HTTPServer.h"

namespace zormid {
	class Zormid {
	public:
		void run();

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

		Config mConfig;
		net::http::HTTPServer mServer;
		std::vector<client::Client> mClients;

		Zormid();
	};
}
