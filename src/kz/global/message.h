#pragma once

#include "utils/json.h"

namespace KZ::API
{

	template<typename Payload>
	struct Message
	{
		u64 id = 0;
		std::string type {};
		Payload payload {};

		Message(const char *type, Payload payload)
		{
			this->type = std::string(type);
			this->payload = payload;
		}

		friend void to_json(json &j, const Message<Payload> &message)
		{
			j["id"] = message.id;
			j["type"] = message.type;
			j["payload"] = message.payload;
		}

		friend void from_json(const json &j, Message<Payload> &message)
		{
			j.at("id").get_to(message.id);
			j.at("type").get_to(message.type);
			j.at("payload").get_to(message.payload);
		}
	};
}; // namespace KZ::API
