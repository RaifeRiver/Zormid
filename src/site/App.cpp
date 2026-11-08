#include "App.h"

#include <optional>

#include "Auth.h"
#include "util/FileHelper.h"

namespace zormid::site {
	void app(const net::http::HTTPRequestHeader& request, net::http::HTTPResponseHeader& response) {
		if (!verifySessionCookies(request)) {
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
