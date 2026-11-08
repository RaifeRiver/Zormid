#include "FileStream.h"

namespace zormid::util {
	IFileStream::IFileStream(std::string path) : mFileStream(path) {}

	void IFileStream::seek(const uint32_t pos) {
		mFileStream.seekg(pos);
	}

	void IFileStream::read(void* buffer, const uint32_t size) {
		mFileStream.read(static_cast<char*>(buffer), size);
	}

	void IFileStream::operator>>(uint32_t& value) {
		read(&value, sizeof(uint32_t));
	}

	IFileStream::~IFileStream() {
		mFileStream.close();
	}

	OFileStream::OFileStream(std::string path) : mFileStream(path) {}

	void OFileStream::write(const void* buffer, const uint32_t size) {
		mFileStream.write(static_cast<const char*>(buffer), size);
	}

	void OFileStream::operator<<(const uint32_t value) {
		write(&value, sizeof(uint32_t));
	}

	OFileStream::~OFileStream() {
		mFileStream.close();
	}
}
