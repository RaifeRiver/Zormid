#include "Auth.h"

#include "user/UserManager.h"
#include "util/FileHelper.h"
#include "util/StringHelper.h"

namespace zormid::site {
	static std::string getPage(const std::string& type, const std::string& message = "") {
		if (const std::optional<std::string> data = util::file::readFile("../res/site/auth.html"); data.has_value()) {
			std::string page = data.value();
			page = util::string::replaceAll(page, "%login-type%", type);
			page = util::string::replaceAll(page, "%response-message%", message.empty() ? "" : "<p id=\"response-error\">" + message + "</p>");
			if (type == "Login") {
				page = util::string::replaceAll(page, "%switch-link%", "<a href=\"?type=signup\">Sign up</a>");
			}
			else if (type == "Sign Up") {
				page = util::string::replaceAll(page, "%switch-link%", "<a href=\"?type=login\">Login</a>");
			}
			return page;
		}
		return "Internal server error";
	}

	void auth(const net::http::HTTPRequestHeader& request, net::http::HTTPResponseHeader& response) {
		if (request.type == "GET") {
			response.statusCode = 200;
			response.statusMessage = "OK";
			response.body = getPage(request.query == "type=signup" ? "Sign Up" : "Login");
		}
		else if (request.type == "POST") {
			const std::string body = request.body;
			const bool signup = request.query == "type=signup";
			const size_t usernameEnd = body.find_first_of('&');
			if (usernameEnd == std::string::npos || !body.starts_with("username=")) {
				response.statusCode = 400;
				response.statusMessage = "Bad Request";
				return;
			}
			const size_t passwordEnd = body.find_first_of('&', usernameEnd + 1);
			const std::string_view passwordView(body.data() + usernameEnd + 1, passwordEnd - usernameEnd - 1);
			if (passwordEnd == std::string::npos || !passwordView.starts_with("password=")) {
				response.statusCode = 400;
				response.statusMessage = "Bad Request";
				return;
			}
			const std::string_view rememberView(body.data() + passwordEnd + 1);
			if (rememberView != "remember=on" && rememberView != "remember=off") {
				response.statusCode = 400;
				response.statusMessage = "Bad Request";
				return;
			}
			const std::string username = body.substr(9, usernameEnd - 9);
			const std::string_view password = passwordView.substr(9);
			const bool remember = rememberView == "remember=on";
			if (signup) {
				if (user::manager::exists(username)) {
					response.statusCode = 409;
					response.statusMessage = "Conflict";
					response.body = getPage("Sign Up", "User already exists");
					return;
				}
				user::manager::create(username, std::string(password));
				response.statusCode = 303;
				response.statusMessage = "See Other";
				response.headers["Location"] = "/app/";
			}
			else {
				if (!user::manager::exists(username)) {
					response.statusCode = 404;
					response.statusMessage = "Not Found";
					response.body = getPage("Login", "User not found");
					return;
				}
				if (!user::manager::verify(username, std::string(password))) {
					response.statusCode = 401;
					response.statusMessage = "Unauthorized";
					response.body = getPage("Login", "Incorrect password");
					return;
				}
				response.statusCode = 303;
				response.statusMessage = "See Other";
				response.headers["Location"] = "/app/";
			}
		}
	}
}
