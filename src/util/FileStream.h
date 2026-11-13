#pragma once

#include <cstdint>
#include <fstream>
#include <string>

namespace zormid::util {
	class IFileStream {
	public:
		explicit IFileStream(const std::string& path);

		void seek(uint32_t pos);

		uint32_t read(void* buffer, uint32_t size);

		void operator>>(uint32_t& value);

		void operator>>(std::string& value);

		~IFileStream();

	private:
		std::ifstream mFileStream;
	};

	class OFileStream {
	public:
		explicit OFileStream(const std::string& path);

		void write(const void* buffer, uint32_t size);

		void operator<<(uint32_t value);

		void operator<<(const std::string& value);

		~OFileStream();

	private:
		std::ofstream mFileStream;
	};
}
