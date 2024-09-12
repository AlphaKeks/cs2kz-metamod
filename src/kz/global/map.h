#pragma once

#include "common.h"
#include "utils/json.h"
#include "mode.h"
#include "player.h"

namespace KZ::API
{
	struct Map
	{
		enum class GlobalStatus;
		struct Mapper;
		struct Course;

		u16 id = 0;
		std::string name {};
		std::optional<std::string> description {};
		GlobalStatus global_status = GlobalStatus::NotGlobal;
		u32 workshop_id = 0;
		std::string checksum {};
		std::vector<Mapper> mappers {};
		std::vector<Course> courses {};
		std::string created_on {};

		enum class GlobalStatus
		{
			NotGlobal = -1,
			InTesting = 0,
			Global = 1,
		};

		struct Mapper
		{
			std::string name {};
			std::string steam_id {};

			NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Mapper, name, steam_id)
		};

		struct Course
		{
			struct Filter;

			u16 id = 0;
			std::optional<std::string> name {};
			std::optional<std::string> description {};
			std::vector<Mapper> mappers {};
			std::vector<Filter> filters {};

			struct Filter
			{
				enum class Tier;
				enum class RankedStatus;

				u16 id = 0;
				Mode mode = Mode::Vanilla;
				bool teleports = false;
				Tier tier = Tier::Impossible;
				RankedStatus ranked_status = RankedStatus::Never;
				std::optional<std::string> notes {};

				enum class Tier
				{
					VeryEasy = 1,
					Easy = 2,
					Medium = 3,
					Advanced = 4,
					Hard = 5,
					VeryHard = 6,
					Extreme = 7,
					Death = 8,
					Unfeasible = 9,
					Impossible = 10,
				};

				enum class RankedStatus
				{
					Never = -1,
					Unranked = 0,
					Ranked = 1,
				};

				friend void from_json(const json &j, Filter &filter)
				{
					j.at("id").get_to(filter.id);
					j.at("teleports").get_to(filter.teleports);

					if (j.at("mode") == "vanilla")
					{
						filter.mode = Mode::Vanilla;
					}
					else if (j.at("mode") == "classic")
					{
						filter.mode = Mode::Classic;
					}

					if (j.at("tier") == "very_easy")
					{
						filter.tier = Tier::VeryEasy;
					}
					else if (j.at("tier") == "easy")
					{
						filter.tier = Tier::Easy;
					}
					else if (j.at("tier") == "medium")
					{
						filter.tier = Tier::Medium;
					}
					else if (j.at("tier") == "advanced")
					{
						filter.tier = Tier::Advanced;
					}
					else if (j.at("tier") == "hard")
					{
						filter.tier = Tier::Hard;
					}
					else if (j.at("tier") == "very_hard")
					{
						filter.tier = Tier::VeryHard;
					}
					else if (j.at("tier") == "extreme")
					{
						filter.tier = Tier::Extreme;
					}
					else if (j.at("tier") == "death")
					{
						filter.tier = Tier::Death;
					}
					else if (j.at("tier") == "unfeasible")
					{
						filter.tier = Tier::Unfeasible;
					}
					else if (j.at("tier") == "impossible")
					{
						filter.tier = Tier::Impossible;
					}

					if (j.at("ranked_status") == "never")
					{
						filter.ranked_status = RankedStatus::Never;
					}
					else if (j.at("ranked_status") == "unranked")
					{
						filter.ranked_status = RankedStatus::Unranked;
					}
					else if (j.at("ranked_status") == "ranked")
					{
						filter.ranked_status = RankedStatus::Ranked;
					}

					if (j.contains("notes"))
					{
						filter.notes = j.at("notes");
					}
				}
			};

			friend void from_json(const json &j, Course &course)
			{
				j.at("id").get_to(course.id);
				j.at("mappers").get_to(course.mappers);
				j.at("filters").get_to(course.filters);

				if (j.contains("name"))
				{
					course.name = j.at("name");
				}

				if (j.contains("description"))
				{
					course.description = j.at("description");
				}
			}
		};

		friend void from_json(const json &j, Map &map)
		{
			j.at("id").get_to(map.id);
			j.at("name").get_to(map.name);
			j.at("workshop_id").get_to(map.workshop_id);
			j.at("checksum").get_to(map.checksum);
			j.at("mappers").get_to(map.mappers);
			j.at("courses").get_to(map.courses);
			j.at("created_on").get_to(map.created_on);

			if (j.at("global_status") == "not_global")
			{
				map.global_status = GlobalStatus::NotGlobal;
			}
			else if (j.at("global_status") == "in_testing")
			{
				map.global_status = GlobalStatus::InTesting;
			}
			else if (j.at("global_status") == "global")
			{
				map.global_status = GlobalStatus::Global;
			}
		}
	};
}; // namespace KZ::API
