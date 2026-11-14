#pragma once

#include <cstdint>
#include <fstream>
#include <string>

#include "user/UserManager.h"

namespace zormid::util {
	class IFileStream {
	public:
		explicit IFileStream(const std::string& path);

		void seek(uint32_t pos);

		uint32_t read(void* buffer, uint32_t size);

		void operator>>(uint32_t& value);

		void operator>>(std::string& value);

		user::manager::UserID readUserID();

		~IFileStream();

	private:
		std::ifstream mFileStream;
	};

	class OFileStream {
	public:
		explicit OFileStream(const std::string& path, bool edit = false);

		void seek(uint32_t pos);

		void seekEnd();

		void write(const void* buffer, uint32_t size);

		void operator<<(uint32_t value);

		void operator<<(bool value);

		void operator<<(const std::string& value);

		void operator<<(const user::manager::UserID& value);

		[[nodiscard]] uint32_t getPosition() const {
			return mPosition;
		}

		~OFileStream();

	private:
		std::ofstream mFileStream;
		uint32_t mPosition = 0;
	};
}
