#pragma once

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

	Tier DeserializeTier(const nlohmann::json &);
} // namespace KZ::API
