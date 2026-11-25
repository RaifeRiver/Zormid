#pragma once

#include <cstdint>
#include <vector>

#include "Channel.h"

namespace zormid::chat {
	struct Message;

	class TextChannel : public Channel {
	public:
		explicit TextChannel(const ChannelID& id);

		uint32_t send(const Message& message);

		[[nodiscard]] uint32_t getMessageCount() const {
			return mMessageCount;
		}

		[[nodiscard]] std::vector<Message> getMessages(uint32_t startIndex, uint32_t count) const;

	protected:
		void deleteChannel_() override;

		void createNewSave_() override;

	private:
		uint32_t mMessageCount = 0;
	};
}
