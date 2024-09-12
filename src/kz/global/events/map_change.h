#pragma once

namespace KZ::API::Events
{
	struct MapChange
	{
		std::string map_name;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(MapChange, map_name)
	};
}; // namespace KZ::API::Events
