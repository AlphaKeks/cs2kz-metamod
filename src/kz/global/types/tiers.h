#pragma once

#include <optional>
#include <string>

#include "vendor/nlohmann/json_fwd.hpp"

namespace KZ::API
{
	enum class Tier
	{
		VERY_EASY = 1,
		EASY,
		MEDIUM,
		ADVANCED,
		HARD,
		VERY_HARD,
		EXTREME,
		DEATH,
		UNFEASIBLE,
		IMPOSSIBLE,
	};

	std::optional<Tier> DeserializeTier(const nlohmann::json &json, std::string &parseError);
} // namespace KZ::API
