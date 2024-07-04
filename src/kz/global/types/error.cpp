#include "vendor/nlohmann/json.hpp"
#include "error.h"
#include "kz/language/kz_language.h"

namespace KZ::API
{
	void Error::Report() const
	{
		META_CONPRINTF("[KZ] API request failed with code %d: %s (%s)\n", status, message.c_str(), details.is_null() ? "" : details.dump().c_str());
	}

	void Error::Report(KZPlayer *player) const
	{
		player->languageService->PrintChat(true, false, "API Error", status);
		player->languageService->PrintConsole(false, false, "API Error Details", message.c_str(), details.is_null() ? "" : details.dump().c_str());
	}

	std::optional<Error> Error::Deserialize(const nlohmann::json &json, u16 status, std::string &parseError)
	{
		if (!json.is_object())
		{
			parseError = "API error is not a JSON object";
			return std::nullopt;
		}

		if (!json.contains("message"))
		{
			parseError = "API error does not contain a message";
			return std::nullopt;
		}

		if (!json["message"].is_string())
		{
			parseError = "API error message is not a string";
			return std::nullopt;
		}

		auto error = Error {
			.status = status,
			.message = json["message"],
		};

		if (json.contains("details"))
		{
			if (!json["details"].is_object())
			{
				META_CONPRINTF("[KZ] ERROR: API error details are not a JSON object.");
			}
			else
			{
				error.details = json["details"];
			}
		}

		return std::make_optional(error);
	}
} // namespace KZ::API
