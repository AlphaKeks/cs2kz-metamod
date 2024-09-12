#pragma once

namespace KZ::API
{
	struct PlayerInfo
	{
		std::string name;
		u64 steam_id;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(PlayerInfo, name, steam_id)
	};
}; // namespace KZ::API
