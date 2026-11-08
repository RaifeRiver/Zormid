#include "Zormid.h"

#include <unistd.h>

#include "client/Client.h"
#include "net/Socket.h"
#include "net/http/HTTPServer.h"
#include "net/http/WebSocket.h"
#include "site/App.h"
#include "site/Auth.h"
#include "user/UserManager.h"
#include "util/FileHelper.h"

namespace zormid {
	void Zormid::run() {
		while (true) {
			mServer.accept();
			for (uint32_t i = 0; i < mClients.size(); i++) {
				mClients[i].process();
				if (!mClients[i].isOpen()) {
					mClients.erase(mClients.begin() + i--);
				}
			}
			usleep(10000);
		}
	}

	void Zormid::init() {
		sInstance = std::unique_ptr<Zormid>(new Zormid());
	}

	Zormid::Zormid() : mServer(8082) {
		user::manager::init();
		mServer.setClientHandler([&](net::http::HTTPRequestHeader request, std::unique_ptr<net::Socket> client) {
			net::http::HTTPResponseHeader response;
			if (request.headers["Connection"] == "Upgrade" && request.headers["Upgrade"] == "websocket") {
				mClients.emplace_back(std::make_unique<net::http::WebSocket>(std::move(client), request));
				return;
			}
			if (request.path.starts_with("/app/")) {
				site::app(request, response);
				response.write(*client);
				client->close();
				return;
			}
			if (request.path == "/auth") {
				site::auth(request, response);
				response.write(*client);
				client->close();
				return;
			}
			if (request.path.starts_with("/friend/")) {
				response.statusCode = 302;
				response.statusMessage = "Found";
				response.headers["Location"] = "/app/";
				response.headers["Set-Cookie"] = "add-friend=" + std::string(request.path.substr(8)) + "; Path=/";
				response.write(*client);
				client->close();
				return;
			}
			const std::optional<std::string> data = util::file::readFile("../res/site/" + request.path);
			if (!data.has_value()) {
				response.statusCode = 404;
				response.statusMessage = "Not Found";
				response.write(*client);
				client->close();
				return;
			}
			response.statusCode = 200;
			response.statusMessage = "OK";
			response.body = data.value();
			std::string mimeType = "text/plain";
			if (request.path.ends_with(".js")) {
				mimeType = "text/javascript";
			}
			else if (request.path.ends_with(".css")) {
				mimeType = "text/css";
			}
			response.headers["Content-Type"] = mimeType;
			response.write(*client);
			client->close();
		});
	}
}
