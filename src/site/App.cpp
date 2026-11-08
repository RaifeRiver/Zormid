#include "App.h"

#include <optional>

#include "util/FileHelper.h"

namespace zormid::site {
	void app(const net::http::HTTPRequestHeader& request, net::http::HTTPResponseHeader& response) {
		std::string userCookie;
		std::string sessionCookie;
		if (const auto cookieIt = request.headers.find("Cookie"); cookieIt != request.headers.end()) {
			const std::string& cookieHeader = cookieIt->second;
			size_t pos = 0;
			while (pos < cookieHeader.length()) {
				size_t end = cookieHeader.find_first_of(';', pos);
				if (end == std::string::npos) {
					end = cookieHeader.length();
				}
				std::string cookie(cookieHeader.substr(pos, cookieHeader.find_first_of('=', pos) - pos));
				if (cookie.starts_with(' ')) {
					cookie = cookie.substr(1);
				}
				if (cookie == "user") {
					userCookie = cookieHeader.substr(pos + 5, end - pos - 6);
				}
				else if (cookie == "sessionID") {
					sessionCookie = cookieHeader.substr(pos + 10, end - pos - 11);
				}
				pos = end + 1;
			}
		}
		if (userCookie.empty() || sessionCookie.empty()) {
			response.statusCode = 302;
			response.statusMessage = "Found";
			response.headers["Location"] = "/auth";
			return;
		}
		if (const std::optional<std::string> data = util::file::readFile("../res/site/app.html"); data.has_value()) {
			response.statusCode = 200;
			response.statusMessage = "OK";
			response.body = *data;
			return;
		}
		response.statusCode = 404;
		response.statusMessage = "Not Found";
	}
}
