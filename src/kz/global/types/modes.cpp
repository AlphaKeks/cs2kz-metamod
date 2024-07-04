#include "vendor/nlohmann/json.hpp"
#include "modes.h"

namespace KZ::API
{
	std::optional<Mode> DeserializeMode(const nlohmann::json &json, std::string &parseError)
	{
		if (!json.is_string())
		{
			parseError = "mode returned by API is not a string.";
			return std::nullopt;
		}

		std::string mode = json;

		if (mode == "vanilla")
		{
			return Mode::VANILLA;
		}
		else if (mode == "classic")
		{
			return Mode::CLASSIC;
		}
		else
		{
			parseError = "unknown mode";
			return std::nullopt;
		}
	}
} // namespace KZ::API
