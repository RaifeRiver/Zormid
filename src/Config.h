#pragma once

#include <cstdint>

namespace zormid {
	class Config {
	public:
		Config();

		[[nodiscard]] uint16_t getPort() const {
			return mPort;
		}

		void setPort(uint16_t port);

		void save() const;

	private:
		uint16_t mPort = 8080;
	};
}
