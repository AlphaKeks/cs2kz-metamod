#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace KZ::API
{
	struct Map
	{
		enum class State : i8
		{
			Invalid = -1,
			InTesting = 0,
			Approved = 1,
		};

		struct Course
		{
			struct Filter
			{
				enum class Tier : u8
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

				enum class State : i8
				{
					Unranked = -1,
					Pending = 0,
					Ranked = 1,
				};

				u16 id;
				Tier nubTier;
				Tier proTier;
				State state;
				std::optional<std::string> notes {};

				bool FromJson(const Json &json)
				{
					if (!json.Get("id", this->id))
					{
						return false;
					}

					std::string nubTier = "";

					if (!json.Get("nub_tier", nubTier))
					{
						return false;
					}

					if (!DecodeTierString(nubTier, this->nubTier))
					{
						return false;
					}

					std::string proTier = "";

					if (!json.Get("pro_tier", proTier))
					{
						return false;
					}

					if (!DecodeTierString(proTier, this->proTier))
					{
						return false;
					}

					std::string state = "";

					if (!json.Get("state", state))
					{
						return false;
					}

					if (!DecodeStateString(state, this->state))
					{
						return false;
					}

					if (!json.Get("notes", this->notes))
					{
						return false;
					}

					return true;
				}

				static bool DecodeTierString(std::string_view tierString, Tier &tier);
				static bool DecodeStateString(std::string_view stateString, State &state);
			};

			struct Filters
			{
				Filter vanilla;
				Filter classic;

				bool FromJson(const Json &json)
				{
					return json.Get("vanilla", this->vanilla) && !json.Get("classic", this->classic);
				}
			};

			u16 id;
			std::string name;
			std::optional<std::string> description {};
			std::vector<PlayerInfo> mappers {};
			Filters filters;

			bool FromJson(const Json &json)
			{
				return json.Get("id", this->id) && json.Get("name", this->name) && json.Get("description", this->description)
					   && json.Get("mappers", this->mappers) && json.Get("filters", this->filters);
			}
		};

		u16 id;
		u32 workshopId;
		std::string name;
		std::optional<std::string> description {};
		State state;
		std::string vpkChecksum;
		std::vector<PlayerInfo> mappers {};
		std::vector<Course> courses {};
		std::string approvedAt;

		bool FromJson(const Json &json)
		{
			if (!json.Get("id", this->id))
			{
				return false;
			}

			if (!json.Get("workshop_id", this->workshopId))
			{
				return false;
			}

			if (!json.Get("name", this->name))
			{
				return false;
			}

			if (!json.Get("description", this->description))
			{
				return false;
			}

			std::string state = "";

			if (!json.Get("state", state))
			{
				return false;
			}

			if (!DecodeStateString(state, this->state))
			{
				return false;
			}

			if (!json.Get("vpk_checksum", this->vpkChecksum))
			{
				return false;
			}

			if (!json.Get("mappers", this->mappers))
			{
				return false;
			}

			if (!json.Get("courses", this->courses))
			{
				return false;
			}

			if (!json.Get("approved_at", this->approvedAt))
			{
				return false;
			}

			return true;
		}

		static bool DecodeStateString(std::string_view stateString, State &state);
	};
} // namespace KZ::API
