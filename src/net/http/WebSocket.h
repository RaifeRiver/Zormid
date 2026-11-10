#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "HTTPHeader.h"
#include "chat/Channel.h"
#include "net/Socket.h"
#include "user/User.h"

namespace zormid::net::http {
	enum class PayloadType {
		TEXT,
		BINARY
	};

	struct PayloadData {
		char* data;
		uint64_t size;
		PayloadType type;
	};

	class PayloadWriter {
	public:
		PayloadWriter& write(const void* data, size_t size);

		PayloadWriter& operator<<(uint32_t data);

		PayloadWriter& operator<<(const std::string& data);

		PayloadWriter& operator<<(const chat::ChannelID& data);

		PayloadWriter& operator<<(const user::Session& data);

		PayloadWriter& operator<<(const user::manager::UserID& data);

		[[nodiscard]] PayloadData buildPayload();

	private:
		std::vector<char> mBuffer;
	};

	class PayloadReader {
	public:
		explicit PayloadReader(const PayloadData& data);

		void read(void* data, size_t size);

		void operator>>(uint32_t& data);

		void operator>>(std::string& data);

		chat::ChannelID readChannelID();

		user::Session readSession();

		user::manager::UserID readUserID();

		~PayloadReader();

	private:
		PayloadData mData;
		size_t mIndex = 0;
	};

	class WebSocket {
	public:
		explicit WebSocket(std::unique_ptr<Socket> socket, HTTPRequestHeader request);

		[[nodiscard]] std::optional<PayloadData> read() const;

		void write(const PayloadData& data) const;

		[[nodiscard]] bool isOpen() const;

		void close() const;

	private:
		std::unique_ptr<Socket> mSocket;
	};
}
