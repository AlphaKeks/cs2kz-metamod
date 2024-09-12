#pragma once

#include "version.h"
#include "common.h"
#include "utils/json.h"
#include "player.h"

namespace KZ::API
{
	struct Hello
	{
		std::string plugin_version = VERSION_STRING;
		std::string current_map {};
		std::vector<PlayerInfo> players {};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Hello, plugin_version, current_map, players)
	};

	struct HelloAck
	{
		f64 heartbeat_interval;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(HelloAck, heartbeat_interval)
	};

}; // namespace KZ::API
