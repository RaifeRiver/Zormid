#include "WebSocket.h"

#include <iostream>

#include "TinySHA1.hpp"

#include "util/Base64.h"

namespace zormid::net::http {
	PayloadWriter& PayloadWriter::write(const void* data, const size_t size) {
		mBuffer.insert(mBuffer.end(), static_cast<const char*>(data), static_cast<const char*>(data) + size);
		return *this;
	}

	PayloadWriter& PayloadWriter::operator<<(const uint32_t data) {
		write(&data, sizeof(uint32_t));
		return *this;
	}

	PayloadWriter& PayloadWriter::operator<<(const std::string& data) {
		*this << static_cast<uint32_t>(data.size());
		write(data.c_str(), data.size());
		return *this;
	}

	PayloadWriter& PayloadWriter::operator<<(const chat::ChannelID& data) {
		write(static_cast<const char*>(data), 20);
		return *this;
	}

	PayloadWriter& PayloadWriter::operator<<(const user::Session& data) {
		write(static_cast<const char*>(data), 20);
		return *this;
	}

	PayloadWriter& PayloadWriter::operator<<(const user::manager::UserID& data) {
		write(static_cast<const char*>(data), 20);
		return *this;
	}

	PayloadData PayloadWriter::buildPayload() {
		PayloadData data = {};
		data.type = PayloadType::BINARY;
		data.size = mBuffer.size();
		data.data = mBuffer.data();
		return data;
	}

	PayloadReader::PayloadReader(const PayloadData& data) : mData(data) {}

	void PayloadReader::read(void* data, const size_t size) {
		if (mIndex + size > mData.size) {
			throw std::out_of_range("Payload read out of bounds");
		}
		memcpy(data, mData.data + mIndex, size);
		mIndex += size;
	}

	void PayloadReader::operator>>(uint32_t& data) {
		if (mIndex + sizeof(uint32_t) > mData.size) {
			throw std::out_of_range("Payload read out of bounds");
		}
		memcpy(&data, mData.data + mIndex, sizeof(uint32_t));
		mIndex += sizeof(uint32_t);
	}

	void PayloadReader::operator>>(std::string& data) {
		uint32_t size;
		*this >> size;
		if (mIndex + size > mData.size) {
			throw std::out_of_range("Payload read out of bounds");
		}
		data.resize(size);
		memcpy(data.data(), mData.data + mIndex, size);
		mIndex += size;
	}

	chat::ChannelID PayloadReader::readChannelID() {
		if (mIndex + 20 > mData.size) {
			throw std::out_of_range("Payload read out of bounds");
		}
		char buffer[20];
		memcpy(buffer, mData.data + mIndex, 20);
		mIndex += 20;
		return chat::ChannelID(buffer);
	}

	user::Session PayloadReader::readSession() {
		if (mIndex + 16 > mData.size) {
			throw std::out_of_range("Payload read out of bounds");
		}
		char buffer[16];
		memcpy(buffer, mData.data + mIndex, 16);
		mIndex += 16;
		return user::Session(buffer);
	}

	user::manager::UserID PayloadReader::readUserID() {
		if (mIndex + 20 > mData.size) {
			throw std::out_of_range("Payload read out of bounds");
		}
		char buffer[20];
		memcpy(buffer, mData.data + mIndex, 20);
		mIndex += 20;
		return user::manager::UserID(buffer);
	}

	PayloadReader::~PayloadReader() {
		free(mData.data);
		mData.size = 0;
	}

	WebSocket::WebSocket(std::unique_ptr<Socket> socket, HTTPRequestHeader request) : mSocket(std::move(socket)) {
		HTTPResponseHeader response;
		response.statusCode = 101;
		response.statusMessage = "Switching Protocols";
		response.headers["Connection"] = "Upgrade";
		response.headers["Upgrade"] = "websocket";
		sha1::SHA1 sha;
		const std::string key = request.headers["Sec-WebSocket-Key"] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		sha.processBlock(key.c_str(), key.c_str() + key.size());
		uint8_t digest[20];
		sha.getDigestBytes(digest);
		response.headers["Sec-WebSocket-Accept"] = util::base64::encode(digest, sizeof(digest));
		response.write(*mSocket);
	}

	std::optional<PayloadData> WebSocket::read() const {
		if (!mSocket->available()) {
			return {};
		}
		uint8_t firstByte;
		mSocket->readBlocking(&firstByte, 1);
		//bool lastMessage = firstByte >> 7 == 1; TODO: implement split packets
		const uint8_t opcode = firstByte & 0xF;
		const PayloadType type = opcode == 1 ? PayloadType::TEXT : PayloadType::BINARY;
		uint8_t payloadLength;
		mSocket->readBlocking(&payloadLength, 1);
		const bool masked = payloadLength >> 7 == 1;
		payloadLength &= 0x7F;
		uint64_t length;
		if (payloadLength < 126) {
			length = payloadLength;
		}
		else if (payloadLength == 126) {
			uint8_t bytes[2];
			mSocket->readBlocking(&bytes, 2);
			length = static_cast<uint16_t>(bytes[0]) << 8 | bytes[1];
		}
		else {
			uint8_t bytes[8];
			mSocket->readBlocking(&bytes, 8);
			length = static_cast<uint64_t>(bytes[0]) << 56 | static_cast<uint64_t>(bytes[1]) << 48 | static_cast<uint64_t>(bytes[2]) << 40 | static_cast<uint64_t>(bytes[3]) << 32 | static_cast<uint64_t>(bytes[4]) << 24 | static_cast<uint64_t>(bytes[5]) << 16 | static_cast<uint64_t>(bytes[6]) << 8 | static_cast<uint64_t>(bytes[7]);
		}
		char mask[4];
		if (masked) {
			mSocket->readBlocking(mask, 4);
		}
		const auto data = static_cast<char*>(malloc(length));
		mSocket->readBlocking(data, length);
		if (masked) {
			for (size_t i = 0; i < length; i++) {
				data[i] = static_cast<char>(data[i] ^ mask[i % 4]);
			}
		}
		PayloadData payload = {.data = data, .size = length, .type = type};
		if (opcode == 0x9) {
			write_(payload, 0xA);
			free(data);
			return {};
		}
		if (opcode == 0x8) {
			free(data);
			if (mClosing) {
				mSocket->close();
			}
			else {
				write_({.size = 0, .type = PayloadType::BINARY}, 0x8);
			}
			return {};
		}
		return payload;
	}

	void WebSocket::write(const PayloadData& data) const {
		write_(data, data.type == PayloadType::TEXT ? 0x1 : 0x2);
	}

	bool WebSocket::isOpen() const {
		return mSocket->isOpen();
	}

	void WebSocket::close() {
		write_({.size = 0, .type = PayloadType::BINARY}, 0x8);
		mClosing = true;
	}

	void WebSocket::write_(const PayloadData& data, const uint8_t opcode) const {
		uint8_t firstByte = 1 << 7;
		firstByte |= opcode;
		mSocket->write(&firstByte, 1);
		uint8_t payloadLength = 0;
		if (data.size < 126) {
			payloadLength = data.size;
		}
		else if (data.size < 65536) {
			payloadLength = 126;
		}
		else {
			payloadLength = 127;
		}
		mSocket->write(&payloadLength, 1);
		if (payloadLength == 126) {
			uint8_t length[2];
			length[0] = static_cast<uint8_t>(data.size >> 8);
			length[1] = static_cast<uint8_t>(data.size & 0xFF);
			mSocket->write(&length, 2);
		}
		else if (payloadLength == 127) {
			uint8_t lengthBytes[8];
			lengthBytes[0] = static_cast<uint8_t>(data.size >> 56);
			lengthBytes[1] = static_cast<uint8_t>(data.size >> 48);
			lengthBytes[2] = static_cast<uint8_t>(data.size >> 40);
			lengthBytes[3] = static_cast<uint8_t>(data.size >> 32);
			lengthBytes[4] = static_cast<uint8_t>(data.size >> 24);
			lengthBytes[5] = static_cast<uint8_t>(data.size >> 16);
			lengthBytes[6] = static_cast<uint8_t>(data.size >> 8);
			lengthBytes[7] = static_cast<uint8_t>(data.size);
			mSocket->write(&lengthBytes, 8);
		}
		if (data.size > 0) {
			mSocket->write(data.data, data.size);
		}
	}
}
