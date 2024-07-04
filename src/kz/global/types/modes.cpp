#include "vendor/nlohmann/json.hpp"
#include "modes.h"
#include "utils/json.h"

namespace KZ::API
{
	std::optional<Mode> DeserializeMode(const nlohmann::json &json, std::string &parseError)
	{
		std::vector<std::pair<std::string, Mode>> expected = {
			std::make_pair("vanilla", Mode::VANILLA),
			std::make_pair("classic", Mode::CLASSIC),
		};

		return KZ::utils::json::ParseEnum(json, parseError, expected);
	}
} // namespace KZ::API
