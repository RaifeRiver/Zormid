#pragma once

#include <memory>

#include "client/Client.h"
#include "net/http/HTTPServer.h"

namespace zormid {
	class Zormid {
	public:
		void run();

		[[nodiscard]] std::vector<client::Client>& getClients() {
			return mClients;
		}

		static void init();

		static Zormid& getInstance() {
			return *sInstance;
		}

	private:
		static inline std::unique_ptr<Zormid> sInstance = nullptr;

		net::http::HTTPServer mServer;
		std::vector<client::Client> mClients;

		Zormid();
	};
}
