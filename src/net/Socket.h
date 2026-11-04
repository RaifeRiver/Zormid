#pragma once

#include <string>
#include <vector>

namespace zormid::net {
	class Socket {
	public:
		explicit Socket(int fd);

		void write(const void* data, size_t size) const;

		template <typename T> void write(const std::vector<T>& data) {
			write(data.data(), data.size() * sizeof(T));
		}

		void writeString(const std::string& str) const;

		size_t read(void* buffer, size_t size);

		std::string readString(char delimiter = '\n');

		void close() const;

	private:
		int mFD;
		char mBuffer[4096] = {};
		size_t mBufferPos = 0;
		size_t mBufferEnd = 0;
	};
}
