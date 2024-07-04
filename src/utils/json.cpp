#include "json.h"

namespace KZ::utils::json
{
	template<typename T>
	std::optional<T> ParseEnum(const nlohmann::json &json, std::string &parseError, std::vector<std::pair<std::string, T>> expectedValues)
	{
		if (!json.is_string())
		{
			parseError = "JSON value was expected to be a string but wasn't";
			return std::nullopt;
		}

		std::string value = json;

		for (const auto &[sValue, tValue] : expectedValues)
		{
			if (value == sValue)
			{
				return tValue;
			}
		}

		parseError = "JSON value did not contain known enum variant";
		return std::nullopt;
	}
} // namespace KZ::utils::json
