#pragma once

#include "common.h"
#include "utils/json.h"
#include "../mode.h"

namespace KZ::API::Events
{
	struct SubmitRecord
	{
		u32 course_id;
		Mode mode;
		u32 styles;
		u32 teleports;
		f64 time;
		u64 player_id;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(SubmitRecord, course_id, mode, styles, teleports, time, player_id)
	};
}; // namespace KZ::API::Events
