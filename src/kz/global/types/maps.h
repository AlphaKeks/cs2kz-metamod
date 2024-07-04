#pragma once

#include <optional>
#include <string>
#include <vector>

#include "vendor/nlohmann/json_fwd.hpp"

#include "common.h"
#include "kz/kz.h"
#include "modes.h"
#include "players.h"
#include "tiers.h"

namespace KZ::API
{
	struct Filter
	{
		enum class RankedStatus : i8
		{
			NEVER = -1,
			UNRANKED = 0,
			RANKED = 1,
		};

		u16 id;
		Mode mode;
		bool teleports;
		Tier tier;
		RankedStatus rankedStatus;
		std::optional<std::string> notes;

		static std::optional<Filter> Deserialize(const nlohmann::json &json, std::string &parseError);
		static std::optional<RankedStatus> DeserializeRankedStatus(const nlohmann::json &json, std::string &parseError);
	};

	struct Course
	{
		u16 id;
		std::optional<std::string> name;
		std::optional<std::string> description;
		std::vector<Player> mappers;
		std::vector<Filter> filters;

		static std::optional<Course> Deserialize(const nlohmann::json &json, std::string &parseError);
	};

	struct Map
	{
		enum class GlobalStatus : i8
		{
			NOT_GLOBAL = -1,
			IN_TESTING = 0,
			GLOBAL = 1,
		};

		u16 id;
		std::string name;
		std::optional<std::string> description;
		GlobalStatus globalStatus;
		u32 workshopID;
		u32 checksum;
		std::vector<Player> mappers;
		std::vector<Course> courses;
		std::string createdOn;

		static std::optional<Map> Deserialize(const nlohmann::json &json, std::string &parseError);
		static std::optional<GlobalStatus> DeserializeGlobalStatus(const nlohmann::json &json, std::string &parseError);

		void Display(KZPlayer *player) const;
	};
} // namespace KZ::API
