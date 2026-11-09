#include "FileStream.h"

namespace zormid::util {
	IFileStream::IFileStream(const std::string& path) : mFileStream(path, std::ios::binary) {}

	void IFileStream::seek(const uint32_t pos) {
		mFileStream.seekg(pos);
	}

	void IFileStream::read(void* buffer, const uint32_t size) {
		mFileStream.read(static_cast<char*>(buffer), size);
	}

	void IFileStream::operator>>(uint32_t& value) {
		read(&value, sizeof(uint32_t));
	}

	void IFileStream::operator>>(std::string& value) {
		uint32_t length;
		*this >> length;
		value.resize(length);
		read(&value[0], length);
	}

	IFileStream::~IFileStream() {
		mFileStream.close();
	}

	OFileStream::OFileStream(const std::string& path) : mFileStream(path, std::ios::binary) {}

	void OFileStream::write(const void* buffer, const uint32_t size) {
		mFileStream.write(static_cast<const char*>(buffer), size);
	}

	void OFileStream::operator<<(const uint32_t value) {
		write(&value, sizeof(uint32_t));
	}

	void OFileStream::operator<<(const std::string& value) {
		*this << static_cast<uint32_t>(value.size());
		write(value.data(), value.size());
	}

	OFileStream::~OFileStream() {
		mFileStream.close();
	}
}
