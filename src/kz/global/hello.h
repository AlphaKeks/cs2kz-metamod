#include "version.h"
#include "common.h"
#include "utils/json.h"

namespace KZ::API
{
	struct PlayerInfo
	{
		std::string name;
		u64 steam_id;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(PlayerInfo, name, steam_id)
	};

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
