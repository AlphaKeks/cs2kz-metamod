#pragma once

namespace KZ::API
{
	struct Preferences
	{
		json preferences {};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Preferences, preferences)
	};
}; // namespace KZ::API
