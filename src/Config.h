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

		[[nodiscard]] int64_t getMaxUploadSize() const {
			return mMaxUploadSize == -1 ? -1 : mMaxUploadSize * 1024 * 1024;
		}

		void setMaxUploadSize(int64_t maxUploadSize);

		void save() const;

	private:
		uint16_t mPort = 8080;
		int64_t mMaxUploadSize = -1;
	};
}
