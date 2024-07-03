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

	Error Error::Deserialize(const nlohmann::json &json, u16 status)
	{
		Error error;
		error.status = status;
		json.at("message").get_to(error.message);

		auto details = json.value("details", nullptr);

		if (details != nullptr)
		{
			error.details = details;
		}

		return error;
	}
} // namespace KZ::API
