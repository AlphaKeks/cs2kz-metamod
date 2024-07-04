#pragma once

#include <optional>
#include <string>

#include "vendor/nlohmann/json_fwd.hpp"
#include "kz/kz.h"

namespace KZ::API
{
	struct Error
	{
		u16 status;
		std::string message;
		nlohmann::json details;

		void Report() const;
		void Report(KZPlayer *player) const;
		static std::optional<Error> Deserialize(const nlohmann::json &, u16 status, std::string &error);
	};
} // namespace KZ::API
