#include "vendor/nlohmann/json.hpp"
#include "tiers.h"

namespace KZ::API
{
	Tier DeserializeTier(const nlohmann::json &json)
	{
		std::string tier;
		json.at("tier").get_to(tier);

		return Tier::EASY;
	}
} // namespace KZ::API
