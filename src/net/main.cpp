#include "http/HTTPServer.h"

int main() {
	zormid::net::http::HTTPServer server(8082);
	server.setClientHandler([](const zormid::net::http::HTTPRequestHeader&, const zormid::net::Socket& client) {
		client.writeString("HTTP/1.1 200 OK\r\n\r\nHello HTTP");
		client.close();
	});
	while (true) {
		server.accept();
	}
}
