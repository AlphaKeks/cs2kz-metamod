#pragma once

namespace KZ::API::Events
{
	struct GetPreferences
	{
		u64 player_id;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GetPreferences, player_id)
	};
}; // namespace KZ::API::Events
