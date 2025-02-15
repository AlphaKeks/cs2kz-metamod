#pragma once

#include <string_view>

#include "kz/global/api.h"

namespace KZ::API::events
{
	struct MapChange
	{
		std::string_view mapName;

		MapChange(std::string_view mapName) : mapName(mapName) {}

		bool ToJson(Json &json) const
		{
			return json.Set("new_map", this->mapName);
		}
	};

	struct MapInfo
	{
		std::optional<KZ::API::Map> data;

		bool FromJson(const Json &json)
		{
			return json.Get("map", this->data);
		}
	};
} // namespace KZ::API::events
