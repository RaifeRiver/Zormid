#include "Socket.h"

#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

namespace zormid::net {
	Socket::Socket(const int fd) : mFD(fd) {}

	void Socket::write(const void* data, const size_t size) const {
		size_t sent = 0;
		while (sent < size) {
			if (const ssize_t s = send(mFD, static_cast<const char*>(data) + sent, size - sent, 0); s > 0) {
				sent += s;
			}
		}
	}

	void Socket::writeString(const std::string& str) {
		ssize_t result = 0;
		size_t sent = 0;
		while ((result = send(mFD, str.c_str() + sent, str.length() - sent, 0)) < str.length()) {
			if (result == -1) {
				if (errno == EPIPE || errno == ECONNRESET) {
					close();
					return;
				}
			}
			else {
				sent += result;
			}
		}
	}

	size_t Socket::read(void* buffer, const size_t size) {
		size_t bufferCopySize = 0;
		if (const size_t bufferSize = mBufferEnd - mBufferPos; bufferSize > 0) {
			bufferCopySize = size > bufferSize ? bufferSize : size;
			memcpy(buffer, mBuffer + mBufferPos, bufferCopySize);
			mBufferPos += bufferCopySize;
			if (bufferCopySize == size) {
				return bufferCopySize;
			}
		}
		const ssize_t read = recv(mFD, static_cast<char*>(buffer) + bufferCopySize, size - bufferCopySize, 0);
		if (read == -1) {
			if (errno == EPIPE || errno == ECONNRESET) {
				close();
			}
			return bufferCopySize;
		}
		return read + bufferCopySize;
	}

	void Socket::readBlocking(void* buffer, const size_t size) {
		size_t s = 0;
		while (s < size) {
			s += read(static_cast<char*>(buffer) + s, size - s);
		}
	}

	std::string Socket::readString(const char delimiter) {
		std::string result;
		while (true) {
			for (size_t i = mBufferPos; i < mBufferEnd; i++) {
				if (mBuffer[i] == delimiter) {
					result.append(mBuffer + mBufferPos, i - mBufferPos);
					mBufferPos = i + 1;
					return result;
				}
			}
			result.append(mBuffer + mBufferPos, mBufferEnd - mBufferPos);
			mBufferPos = 0;
			if (const ssize_t read = recv(mFD, mBuffer, sizeof(mBuffer), 0); read > 0) {
				mBufferEnd = read;
			}
			else if (read == -1) {
				if (errno == EPIPE || errno == ECONNRESET) {
					close();
					return result;
				}
				mBufferEnd = 0;
			}
		}
	}

	bool Socket::available() {
		if (mBufferPos < mBufferEnd) {
			return true;
		}
		mBufferPos = 0;
		mBufferEnd = 0;
		if (const ssize_t read = recv(mFD, mBuffer, sizeof(mBuffer), 0); read > 0) {
			mBufferEnd = read;
			return true;
		}
		else if (read == -1) {
			if (errno == EPIPE || errno == ECONNRESET) {
				close();
				return false;
			}
			mBufferEnd = 0;
		}
		return false;
	}

	void Socket::close() {
		::close(mFD);
		mOpen = false;
	}
}
