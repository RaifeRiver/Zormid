#pragma once
#include "net/http/HTTPHeader.h"

namespace zormid::site {
	void app(const net::http::HTTPRequestHeader& request, net::http::HTTPResponseHeader& response);
}
