#include "utils/json.h"

namespace KZ::API
{
	struct Heartbeat
	{
		std::vector<PlayerInfo> players {};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Heartbeat, players)
	};
}; // namespace KZ::API
