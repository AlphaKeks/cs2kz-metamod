#include "http.h"

extern CSteamGameServerAPIContext g_steamAPI;

namespace HTTP
{
	ISteamHTTP *g_pHTTP = nullptr;
	std::vector<InFlightRequest *> g_InFlightRequests;

	void Request::Send(RequestCallback callback) const
	{
		if (!g_pHTTP)
		{
			META_CONPRINTF("[HTTP] Initializing HTTP client...\n");
			g_steamAPI.Init();
			g_pHTTP = g_steamAPI.SteamHTTP();
		}

		EHTTPMethod volvoMethod;

		switch (method)
		{
			case Method::GET:
				volvoMethod = k_EHTTPMethodGET;
				break;
			case Method::POST:
				volvoMethod = k_EHTTPMethodPOST;
				break;
		}

		auto requestHandle = g_pHTTP->CreateHTTPRequest(volvoMethod, uri.c_str());

		if (method == Method::POST)
		{
			if (!g_pHTTP->SetHTTPRequestRawPostBody(requestHandle, "application/json", (u8 *)body.data(), body.size()))
			{
				META_CONPRINTF("[HTTP] Failed to set request body.\n");
				return;
			}
		}

		for (const auto &[name, value] : headers)
		{
			g_pHTTP->SetHTTPRequestHeaderValue(requestHandle, name.c_str(), value.c_str());
		}

		SteamAPICall_t steamCallHandle;

		if (!g_pHTTP->SendHTTPRequest(requestHandle, &steamCallHandle))
		{
			META_CONPRINTF("[HTTP] Failed to send HTTP request.\n");
			return;
		}

		new InFlightRequest(requestHandle, steamCallHandle, uri, body, callback);
	}

	std::optional<std::string> Response::Header(const char *name) const
	{
		u32 headerValueSize;

		if (!g_pHTTP->GetHTTPResponseHeaderSize(requestHandle, name, &headerValueSize))
		{
			return std::nullopt;
		}

		u8 *rawHeaderValue = new u8[headerValueSize + 1];

		if (!g_pHTTP->GetHTTPResponseHeaderValue(requestHandle, name, rawHeaderValue, headerValueSize))
		{
			delete[] rawHeaderValue;
			return std::nullopt;
		}

		rawHeaderValue[headerValueSize] = '\0';
		std::string headerValue = std::string((char *)rawHeaderValue);
		delete[] rawHeaderValue;

		return headerValue;
	}

	std::optional<std::string> Response::Body() const
	{
		u32 bodySize;

		if (!g_pHTTP->GetHTTPResponseBodySize(requestHandle, &bodySize))
		{
			return std::nullopt;
		}

		u8 *rawBody = new u8[bodySize + 1];

		if (!g_pHTTP->GetHTTPResponseBodyData(requestHandle, rawBody, bodySize))
		{
			delete[] rawBody;
			return std::nullopt;
		}

		rawBody[bodySize] = '\0';
		std::string body = std::string((char *)rawBody);
		delete[] rawBody;

		return body;
	}

	void InFlightRequest::OnRequestCompleted(HTTPRequestCompleted_t *completedRequest, bool failed)
	{
		if (failed)
		{
			META_CONPRINTF("[KZ::HTTP] request to `%s` failed with code %d\n", uri.c_str(), completedRequest->m_eStatusCode);
			delete this;
			return;
		}

		Response response(completedRequest->m_eStatusCode, completedRequest->m_hRequest);
		callback(response);

		if (g_pHTTP)
		{
			g_pHTTP->ReleaseHTTPRequest(completedRequest->m_hRequest);
		}

		delete this;
	}
}; // namespace HTTP
