#pragma once

#include <optional>
#include <string>
#include <vector>

#include "vendor/nlohmann/json_fwd.hpp"

#include "common.h"
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
		std::string notes;

		static Filter Deserialize(const nlohmann::json &);
		static RankedStatus DeserializeRankedStatus(const nlohmann::json &);
	};

	struct Course
	{
		u16 id;
		std::string name;
		std::string description;
		std::vector<Player> mappers;
		std::vector<Filter> filters;

		static Course Deserialize(const nlohmann::json &);
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
		std::string description;
		GlobalStatus globalStatus;
		u32 workshopID;
		u32 checksum;
		std::vector<Player> mappers;
		std::vector<Course> courses;
		std::string createdOn;

		static Map Deserialize(const nlohmann::json &);
		static GlobalStatus DeserializeGlobalStatus(const nlohmann::json &);
	};
} // namespace KZ::API
