#include "FileStream.h"

namespace zormid::util {
	IFileStream::IFileStream(const std::string& path) : mFileStream(path, std::ios::binary) {}

	void IFileStream::seek(const uint32_t pos) {
		mFileStream.seekg(pos);
	}

	uint32_t IFileStream::read(void* buffer, const uint32_t size) {
		mFileStream.read(static_cast<char*>(buffer), size);
		return mFileStream.gcount();
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

	user::manager::UserID IFileStream::readUserID() {
		char buffer[20];
		read(buffer, 20);
		return user::manager::UserID(buffer);
	}

	IFileStream::~IFileStream() {
		mFileStream.close();
	}

	OFileStream::OFileStream(const std::string& path, bool edit) : mFileStream(path, edit ? std::ios::binary | std::ios::in : std::ios::binary) {}

	void OFileStream::seek(const uint32_t pos) {
		mFileStream.seekp(pos);
		mPosition = pos;
	}

	void OFileStream::seekEnd() {
		mFileStream.seekp(0, std::ios::end);
		mPosition = static_cast<uint32_t>(mFileStream.tellp());
	}

	void OFileStream::write(const void* buffer, const uint32_t size) {
		mFileStream.write(static_cast<const char*>(buffer), size);
		mPosition += size;
	}

	void OFileStream::operator<<(const uint32_t value) {
		write(&value, sizeof(uint32_t));
	}

	void OFileStream::operator<<(const bool value) {
		write(&value, sizeof(bool));
	}

	void OFileStream::operator<<(const std::string& value) {
		*this << static_cast<uint32_t>(value.size());
		write(value.data(), value.size());
	}

	void OFileStream::operator<<(const user::manager::UserID& value) {
		write(static_cast<const char*>(value), 20);
	}

	OFileStream::~OFileStream() {
		mFileStream.close();
	}
}
