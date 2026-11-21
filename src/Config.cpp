#include "Config.h"

#include <fstream>

#include "util/StringHelper.h"

namespace zormid {
	Config::Config() {
		std::ifstream file("../data/zormid.conf");
		if (!file.is_open()) {
			save();
			return;
		}
		std::string line;
		while (std::getline(file, line)) {
			if (line.starts_with('#')) {
				continue;
			}
			std::vector<std::string> splitLine = util::string::split(line, "=");
			if (splitLine.size() != 2) {
				continue;
			}
			if (splitLine[0] == "port") {
				if (const int port = std::stoi(splitLine[1]); port > 0 && port < 65535) {
					mPort = port;
				}
			}
			if (splitLine[0] == "max_upload_size") {
				try {
					mMaxUploadSize = std::stoll(splitLine[1]);
				}
				catch (std::exception&) {}
				if (mMaxUploadSize < -1) {
					mMaxUploadSize = -1;
				}
			}
		}
		save();
	}

	void Config::setPort(const uint16_t port) {
		mPort = port;
	}

	void Config::setMaxUploadSize(const int64_t maxUploadSize) {
		mMaxUploadSize = maxUploadSize / 1024 / 1024;
	}

	void Config::save() const {
		std::ofstream file("../data/zormid.conf");
		if (!file.is_open()) {
			return;
		}
		file << "# The port of the web server used to connect to Zormid" << std::endl;
		file << "port=" << mPort << std::endl;
		file << std::endl;
		file << "# The maximum file size in megabytes to allow uploading" << std::endl;
		file << "# Set to -1 to disable limit and 0 to disable uploading" << std::endl;
		file << "max_upload_size=" << mMaxUploadSize << std::endl;
	}
}
