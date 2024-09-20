#pragma once

namespace KZ::API::Events
{
	struct PlayerSession
	{
		u64 steam_id;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(PlayerSession, steam_id, session)
	};
}; // namespace KZ::API::Events
