#pragma once

#include <string>

#include "vendor/nlohmann/json_fwd.hpp"

namespace KZ::API
{
	struct Player
	{
		std::string name;
		std::string steamID;

		static Player Deserialize(const nlohmann::json &);
	};

	struct FullPlayer
	{
		std::string name;
		std::string steamID;
		bool isBanned;

		static FullPlayer Deserialize(const nlohmann::json &);
	};
} // namespace KZ::API
