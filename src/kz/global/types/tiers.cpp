#include "vendor/nlohmann/json.hpp"
#include "tiers.h"
#include "utils/json.h"

namespace KZ::API
{
	std::optional<Tier> DeserializeTier(const nlohmann::json &json, std::string &parseError)
	{
		// clang-format off
		std::vector<std::pair<std::string, Tier>> expected = {
			std::make_pair("very_easy", Tier::VERY_EASY),
			std::make_pair("easy", Tier::EASY),
			std::make_pair("medium", Tier::MEDIUM),
			std::make_pair("advanced", Tier::ADVANCED),
			std::make_pair("hard", Tier::HARD),
			std::make_pair("very_hard", Tier::VERY_HARD),
			std::make_pair("extreme", Tier::EXTREME),
			std::make_pair("death", Tier::DEATH),
			std::make_pair("unfeasible", Tier::UNFEASIBLE),
			std::make_pair("impossible", Tier::IMPOSSIBLE),
		};
		// clang-format on

		return KZ::utils::json::ParseEnum(json, parseError, expected);
	}
} // namespace KZ::API
