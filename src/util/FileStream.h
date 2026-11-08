#pragma once

#include <cstdint>
#include <fstream>
#include <string>

namespace zormid::util {
	class IFileStream {
	public:
		explicit IFileStream(std::string path);

		void seek(uint32_t pos);

		void read(void* buffer, uint32_t size);

		void operator>>(uint32_t& value);

		~IFileStream();

	private:
		std::ifstream mFileStream;
	};

	class OFileStream {
	public:
		explicit OFileStream(std::string path);

		void write(const void* buffer, uint32_t size);

		void operator<<(uint32_t value);

		~OFileStream();

	private:
		std::ofstream mFileStream;
	};
}
