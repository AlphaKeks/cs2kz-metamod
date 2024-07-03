#pragma once

#include <string>

#include "vendor/nlohmann/json_fwd.hpp"

namespace KZ::API
{
	struct Player
	{
		std::string name;
		std::string steamID;
		bool isBanned;

		nlohmann::json Serialize() const;
		static Player Deserialize(const nlohmann::json &);
	};
} // namespace KZ::API
