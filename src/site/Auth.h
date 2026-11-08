#pragma once

#include "net/http/HTTPHeader.h"

namespace zormid::site {
	void auth(const net::http::HTTPRequestHeader& request, net::http::HTTPResponseHeader& response);
}
