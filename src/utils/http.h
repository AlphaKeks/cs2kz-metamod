#pragma once

#undef snprintf
#include <steam/steam_gameserver.h>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.h"

namespace HTTP
{
	enum class Method;
	class Request;
	class Response;
	class InFlightRequest;

	typedef u16 StatusCode;
	typedef std::unordered_map<std::string, std::string> HeaderMap;
	typedef std::function<void(Response)> RequestCallback;

	extern std::vector<InFlightRequest *> g_InFlightRequests;

	/**
	 * HTTP methods.
	 */
	enum class Method
	{
		GET,
		POST,
	};

	class Request
	{
	public:
		Request(Method method, std::string uri) : method(method), uri(uri) {}

		/**
		 * Adds a query parameter to the URI.
		 */
		void AddQueryParam(const char *key, const char *value)
		{
			uri += hasQueryParams ? "&" : "?";
			uri += key;
			uri += "=";
			uri += value;
			hasQueryParams = true;
		}

		/**
		 * Adds a header to the request.
		 */
		void AddHeader(const char *name, const char *value)
		{
			headers[name] = value;
		}

		/**
		 * Sets the request body.
		 */
		void SetBody(std::string body)
		{
			this->body = body;
		}

		/**
		 * Send the request.
		 *
		 * The given `callback` will be executed once a response has been received.
		 */
		void Send(RequestCallback callback) const;

	private:
		Method method;
		std::string uri;

		// We append query parameters to the URI directly, so we need to keep
		// track of whether we already have any.
		// If we don't, we need to append `?key=value`, otherwise `&key=value`.
		bool hasQueryParams {};

		HeaderMap headers {};
		std::string body {};
	};

	class Response
	{
	public:
		Response(StatusCode statusCode, HTTPRequestHandle requestHandle) : statusCode(statusCode), requestHandle(requestHandle) {}

		/**
		 * Returns the HTTP status code of this response.
		 */
		StatusCode StatusCode() const
		{
			return statusCode;
		}

		/**
		 * Returns a header with the given `name` from the response headers.
		 */
		std::optional<std::string> Header(const char *name) const;

		/**
		 * Returns the response body.
		 *
		 * If there is no response body, a nullptr is returned.
		 */
		std::optional<std::string> Body() const;

	private:
		HTTP::StatusCode statusCode;
		HTTPRequestHandle requestHandle;
	};

	class InFlightRequest
	{
	public:
		InFlightRequest(const InFlightRequest &req) = delete;

		InFlightRequest(HTTPRequestHandle handle, SteamAPICall_t steamCallHandle, std::string uri, std::string body, RequestCallback callback)
			: uri(uri), body(body), handle(handle), callback(callback)
		{
			callResult.SetGameserverFlag();
			callResult.Set(steamCallHandle, this, &InFlightRequest::OnRequestCompleted);
			g_InFlightRequests.push_back(this);
		}

		~InFlightRequest()
		{
			for (auto req = g_InFlightRequests.begin(); req != g_InFlightRequests.end(); req++)
			{
				if (*req == this)
				{
					g_InFlightRequests.erase(req);
					break;
				}
			}
		}

	private:
		std::string uri;
		std::string body;
		HTTPRequestHandle handle;
		CCallResult<InFlightRequest, HTTPRequestCompleted_t> callResult;
		RequestCallback callback;

		void OnRequestCompleted(HTTPRequestCompleted_t *completedRequest, bool failed);
	};
}; // namespace HTTP
