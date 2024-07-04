#include "vendor/nlohmann/json.hpp"
#include "modes.h"

namespace KZ::API
{
	Mode DeserializeMode(const nlohmann::json &json)
	{
		std::string mode;
		json.at("mode").get_to(mode);

		if (mode == "vanilla")
		{
			return Mode::VANILLA;
		}
		else
		{
			return Mode::CLASSIC;
		}
	}
} // namespace KZ::API
