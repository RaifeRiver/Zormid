#include <unistd.h>

#include "client/Client.h"
#include "net/http/HTTPServer.h"
#include "net/http/WebSocket.h"
#include "util/FileHelper.h"

int main() {
	zormid::net::http::HTTPServer server(8082);
	std::vector<zormid::client::Client> clients;
	server.setClientHandler([&](zormid::net::http::HTTPRequestHeader request, std::unique_ptr<zormid::net::Socket> client) {
		zormid::net::http::HTTPResponseHeader response;
		if (request.headers["Connection"] == "Upgrade" && request.headers["Upgrade"] == "websocket") {
			clients.emplace_back(std::make_unique<zormid::net::http::WebSocket>(std::move(client), request));
			return;
		}
		if (request.path.find("..") != std::string::npos) {
			response.statusCode = 403;
			response.statusMessage = "Forbidden";
			response.write(*client);
			client->close();
			return;
		}
		const std::optional<std::string> data = zormid::util::file::readFile("../res/site/" + request.path);
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
		response.write(*client);
		client->close();
	});
	while (true) {
		server.accept();
		for (uint32_t i = 0; i < clients.size(); i++) {
			clients[i].process();
			if (clients[i].isClosed()) {
				clients.erase(clients.begin() + i--);
			}
		}
		usleep(10000);
	}
}
