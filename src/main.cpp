#include <unistd.h>

#include "TinySHA1.hpp"
#include "net/http/HTTPServer.h"
#include "util/Base64.h"
#include "util/FileHelper.h"

int main() {
	zormid::net::http::HTTPServer server(8082);
	server.setClientHandler([](zormid::net::http::HTTPRequestHeader request, const zormid::net::Socket& client) {
		zormid::net::http::HTTPResponseHeader response;
		if (request.headers["Connection"] == "Upgrade" && request.headers["Upgrade"] == "websocket") {
			response.statusCode = 101;
			response.statusMessage = "Switching Protocols";
			response.headers["Connection"] = "Upgrade";
			response.headers["Upgrade"] = "websocket";
			sha1::SHA1 sha;
			std::string key = request.headers["Sec-WebSocket-Key"] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			sha.processBlock(key.c_str(), key.c_str() + key.size());
			uint8_t digest[20];
			sha.getDigestBytes(digest);
			response.headers["Sec-WebSocket-Accept"] = zormid::util::base64::encode(digest, sizeof(digest));
			response.write(client);
			client.close();
			return;
		}
		if (request.path.find("..") != std::string::npos) {
			response.statusCode = 403;
			response.statusMessage = "Forbidden";
			response.write(client);
			client.close();
			return;
		}
		const std::optional<std::string> data = zormid::util::file::readFile("../res/site/" + request.path);
		if (!data.has_value()) {
			response.statusCode = 404;
			response.statusMessage = "Not Found";
			response.write(client);
			client.close();
			return;
		}
		response.statusCode = 200;
		response.statusMessage = "OK";
		response.body = data.value();
		response.write(client);
		client.close();
	});
	while (true) {
		server.accept();
		usleep(10000);
	}
}
