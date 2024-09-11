#include "utils/json.h"

namespace KZ::API
{
	template<typename Payload>
	struct Message
	{
		u64 id;
		std::string type;
		Payload payload;

		friend void to_json(json &j, const Message<Payload> &message)
		{
			j = message.payload;
			j["id"] = message.id;
			j["type"] = message.type;
		}

		friend void from_json(const json &j, Message<Payload> &message)
		{
			message.payload.from_json(j);
			j.at("id").get_to(message.id);
			j.at("type").get_to(message.type);
		}
	};
}; // namespace KZ::API
