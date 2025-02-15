#pragma once

#include <chrono>
#include <optional>
#include <string_view>

#include "version.h"
#include "kz/global/api.h"

namespace KZ::API::handshake
{
	struct Hello
	{
		struct PlayerInfo
		{
			u64 id;
			std::string_view name;
		};

		std::string_view checksum;
		std::string_view currentMapName;
		std::unordered_map<u64, PlayerInfo> players;

		Hello(std::string_view checksum, std::string_view currentMapName) : checksum(checksum), currentMapName(currentMapName) {}

		void AddPlayer(u64 id, std::string_view name)
		{
			this->players[id] = {id, name};
		}

		bool ToJson(Json &json) const
		{
			return json.Set("plugin_version", VERSION_STRING) && json.Set("plugin_version_checksum", this->checksum)
				   && json.Set("map", this->currentMapName) && json.Set("players", this->players);
		}
	};

	struct HelloAck
	{
		std::chrono::seconds heartbeatInterval;
		std::optional<KZ::API::Map> mapInfo;

		bool FromJson(const Json &json)
		{
			f64 heartbeatInterval;

			if (!json.Get("heartbeat_interval", heartbeatInterval))
			{
				return false;
			}

			this->heartbeatInterval = heartbeatInterval;

			return json.Get("map", this->mapInfo);
		}
	};
} // namespace KZ::API::handshake
