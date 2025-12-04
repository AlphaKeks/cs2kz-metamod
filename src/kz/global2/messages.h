#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace KZ::api::messages
{
	struct MapInfo
	{
		u16 id;
		std::string name;

		inline bool FromJson(const Json &json)
		{
			return json.Get("id", this->id) && json.Get("name", this->name);
		}
	};

	namespace handshake
	{
		struct Hello
		{
			std::string_view pluginVersion;
			std::string_view pluginVersionChecksum;
			std::string_view currentMap;

			inline bool ToJson(Json &json) const
			{
				return json.Set("plugin_version", this->pluginVersion) && json.Set("plugin_version_checksum", this->pluginVersionChecksum)
					   && json.Set("current_map", this->currentMap);
			}
		};

		struct ModeChecksums
		{
			struct Checksums
			{
				std::string linux;
				std::string windows;

				inline bool FromJson(const Json &json)
				{
					return json.Get("linux", this->linux) && json.Get("windows", this->windows);
				}
			};

			Checksums vanilla;
			Checksums classic;

			inline bool FromJson(const Json &json)
			{
				return json.Get("vanilla_cs2", this->vanilla) && json.Get("classic", this->classic);
			}
		};

		struct HelloAck
		{
			std::string sessionID;
			f64 heartbeatInterval;
			std::optional<MapInfo> mapInfo;
			ModeChecksums modeChecksums;

			inline bool FromJson(const Json &json)
			{
				if (!json.Get("session_id", this->sessionID) || !json.Get("heartbeat_interval", this->heartbeatInterval))
				{
					return false;
				}

				if (json.ContainsKey("map_info") && !json.Get("map_info", this->mapInfo))
				{
					return false;
				}

				if (!json.ContainsKey("mode_checksums") || !json.Get("mode_checksums", this->modeChecksums))
				{
					return false;
				}

				return true;
			}
		};

	}; // namespace handshake

	struct Error
	{
		std::string message;

		inline bool FromJson(const Json &json)
		{
			return json.Get("error", this->message);
		}
	};

	struct MapChange
	{
		std::string_view name;

		static const char *Name()
		{
			return "map_change";
		}

		inline bool ToJson(Json &json) const
		{
			return json.Set("name", this->name);
		}
	};

	struct PlayerJoin
	{
		std::string_view id;
		std::string_view name;
		std::string_view ipAddress;

		static const char *Name()
		{
			return "player_join";
		}

		inline bool ToJson(Json &json) const
		{
			return json.Set("id", this->id) && json.Set("name", this->name) && json.Set("ip_address", this->ipAddress);
		}
	};

	struct PlayerJoinAck
	{
		Json preferences;
		bool isBanned;

		inline bool FromJson(const Json &json)
		{
			return json.Get("preferences", this->preferences) && json.Get("is_banned", this->isBanned);
		}
	};

	struct PlayerLeave
	{
		std::string_view id;
		std::string_view name;
		std::optional<Json> preferences;

		static const char *Name()
		{
			return "player_leave";
		}

		inline bool ToJson(Json &json) const
		{
			return json.Set("id", this->id) && json.Set("name", this->name) && json.Set("preferences", this->preferences);
		}
	};
}; // namespace KZ::api::messages
