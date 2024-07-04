#pragma once

#include <optional>
#include <vector>
#include "vendor/nlohmann/json.hpp"

namespace KZ::utils::json
{
	template<typename T>
	std::optional<T> ParseEnum(const nlohmann::json &json, std::string &parseError, std::vector<std::pair<std::string, T>> expectedValues);
} // namespace KZ::utils::json
