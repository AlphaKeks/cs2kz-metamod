#pragma once

#include "../game_session.h"

namespace KZ::API::Events
{
	struct PlayerUpdate
	{
		PlayerInfo player;
		json preferences {};
		GameSession session {};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(PlayerUpdate, player, preferences, session)
	};
}; // namespace KZ::API::Events
