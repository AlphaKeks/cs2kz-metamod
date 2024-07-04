#pragma once

#include <optional>
#include <string>

#include "vendor/nlohmann/json_fwd.hpp"

namespace KZ::API
{
	enum class Mode
	{
		VANILLA = 1,
		CLASSIC = 2,
	};

	std::optional<Mode> DeserializeMode(const nlohmann::json &json, std::string &parseError);
} // namespace KZ::API
