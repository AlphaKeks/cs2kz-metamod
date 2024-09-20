#pragma once

#include "../player.h"

namespace KZ::API::Events
{
	struct PlayerCountChange
	{
		std::vector<PlayerInfo> authenticated_players {};
		u64 total_players = 0;
		u64 max_players = 0;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(PlayerCountChange, authenticated_players, total_players, max_players)
	};
}; // namespace KZ::API::Events
