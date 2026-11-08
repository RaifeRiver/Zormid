#pragma once

#include "net/http/HTTPHeader.h"

namespace zormid::site {
	bool verifySessionCookies(const net::http::HTTPRequestHeader& request);

	void auth(const net::http::HTTPRequestHeader& request, net::http::HTTPResponseHeader& response);
}
