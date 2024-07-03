#include "vendor/nlohmann/json.hpp"
#include "players.h"

namespace KZ::API
{
	Player Player::Deserialize(const nlohmann::json &json)
	{
		Player player;
		json.at("name").get_to(player.name);
		json.at("steam_id").get_to(player.steamID);
		return player;
	}

	FullPlayer FullPlayer::Deserialize(const nlohmann::json &json)
	{
		FullPlayer player;
		json.at("name").get_to(player.name);
		json.at("steam_id").get_to(player.steamID);
		json.at("is_banned").get_to(player.isBanned);
		return player;
	}
} // namespace KZ::API
