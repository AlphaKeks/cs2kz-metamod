#include "vendor/nlohmann/json.hpp"
#include "players.h"

namespace KZ::API
{
	std::optional<Player> Player::Deserialize(const nlohmann::json &json, std::string &parseError)
	{
		if (!json.is_object())
		{
			parseError = "player is not a JSON object.";
			return std::nullopt;
		}

		if (!json.contains("name"))
		{
			parseError = "player returned by API does not have a name.";
			return std::nullopt;
		}

		if (!json["name"].is_string())
		{
			parseError = "player name is not a string.";
			return std::nullopt;
		}

		std::string name = json["name"];

		if (!json.contains("steam_id"))
		{
			parseError = "player returned by API does not have a SteamID.";
			return std::nullopt;
		}

		if (!json["steam_id"].is_string())
		{
			parseError = "player SteamID is not a string.";
			return std::nullopt;
		}

		std::string steamID = json["steam_id"];

		return Player {
			.name = name,
			.steamID = steamID,
		};
	}

	std::optional<FullPlayer> FullPlayer::Deserialize(const nlohmann::json &json, std::string &parseError)
	{
		if (!json.is_object())
		{
			parseError = "player is not a JSON object.";
			return std::nullopt;
		}

		if (!json.contains("name"))
		{
			parseError = "player returned by API does not have a name.";
			return std::nullopt;
		}

		if (!json["name"].is_string())
		{
			parseError = "player name is not a string.";
			return std::nullopt;
		}

		std::string name = json["name"];

		if (!json.contains("steam_id"))
		{
			parseError = "player returned by API does not have a SteamID.";
			return std::nullopt;
		}

		if (!json["steam_id"].is_string())
		{
			parseError = "player SteamID is not a string.";
			return std::nullopt;
		}

		std::string steamID = json["steam_id"];

		if (!json.contains("is_banned"))
		{
			parseError = "player returned by API does not have an `is_banned` field.";
			return std::nullopt;
		}

		if (!json["is_banned"].is_boolean())
		{
			parseError = "player `is_banned` field is not a bool.";
			return std::nullopt;
		}

		bool isBanned = json["is_banned"];

		return FullPlayer {
			.name = name,
			.steamID = steamID,
			.isBanned = isBanned,
		};
	}
} // namespace KZ::API
