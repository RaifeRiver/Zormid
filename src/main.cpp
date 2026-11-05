#include <unistd.h>

#include "net/http/HTTPServer.h"
#include "util/FileHelper.h"

int main() {
	zormid::net::http::HTTPServer server(8082);
	server.setClientHandler([](const zormid::net::http::HTTPRequestHeader& request, const zormid::net::Socket& client) {
		if (request.path.find("..") != std::string::npos) {
			client.writeString("HTTP/1.1 403 Forbidden");
			client.close();
			return;
		}
		const std::optional<std::string> data = zormid::util::file::readFile("../res/site/" + request.path);
		if (!data.has_value()) {
			client.writeString("HTTP/1.1 404 Not Found");
			client.close();
			return;
		}
		client.writeString("HTTP/1.1 200 OK\r\n\r\n" + *data);
		client.close();
	});
	while (true) {
		server.accept();
		usleep(10000);
	}
}
