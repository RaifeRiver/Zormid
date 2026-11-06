#include "WebSocket.h"

#include "TinySHA1.hpp"

#include "util/Base64.h"

namespace zormid::net::http {
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
		bool lastMessage = firstByte >> 7 == 1;
		uint8_t opcode = firstByte & 0xF;
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
		return {{.data = data, .size = length}};
	}

	void WebSocket::close() const {
		mSocket->close();
	}
}
