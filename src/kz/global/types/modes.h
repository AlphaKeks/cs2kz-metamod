#pragma once

#include "vendor/nlohmann/json_fwd.hpp"

namespace KZ::API
{
	enum class Mode
	{
		VANILLA = 1,
		CLASSIC = 2,
	};

	Mode DeserializeMode(const nlohmann::json &);
} // namespace KZ::API
