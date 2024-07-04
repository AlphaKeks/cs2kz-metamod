#include "vendor/nlohmann/json.hpp"
#include "tiers.h"

namespace KZ::API
{
	std::optional<Tier> DeserializeTier(const nlohmann::json &json, std::string &parseError)
	{
		if (!json.is_string())
		{
			parseError = "tier returned by API is not a string";
			return std::nullopt;
		}

		std::string tier = json;

		if (tier == "very_easy")
		{
			return Tier::VERY_EASY;
		}
		else if (tier == "easy")
		{
			return Tier::EASY;
		}
		else if (tier == "medium")
		{
			return Tier::MEDIUM;
		}
		else if (tier == "advanced")
		{
			return Tier::ADVANCED;
		}
		else if (tier == "hard")
		{
			return Tier::HARD;
		}
		else if (tier == "very_hard")
		{
			return Tier::VERY_HARD;
		}
		else if (tier == "extreme")
		{
			return Tier::EXTREME;
		}
		else if (tier == "death")
		{
			return Tier::DEATH;
		}
		else if (tier == "unfeasible")
		{
			return Tier::UNFEASIBLE;
		}
		else if (tier == "impossible")
		{
			return Tier::IMPOSSIBLE;
		}
		else
		{
			parseError = "unknown tier";
			return std::nullopt;
		}
	}
} // namespace KZ::API
