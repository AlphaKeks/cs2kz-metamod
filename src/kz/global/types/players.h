#pragma once

#include <optional>
#include <string>

#include "vendor/nlohmann/json_fwd.hpp"

namespace KZ::API
{
	struct Player
	{
		std::string name;
		std::string steamID;

		static std::optional<Player> Deserialize(const nlohmann::json &json, std::string &parseError);
	};

	struct FullPlayer
	{
		std::string name;
		std::string steamID;
		bool isBanned;

		static std::optional<FullPlayer> Deserialize(const nlohmann::json &json, std::string &parseError);
	};
} // namespace KZ::API
