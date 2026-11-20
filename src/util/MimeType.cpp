#include "MimeType.h"

namespace zormid::util {
	std::string getMimeType(const std::string& extension) {
		if (extension == "css") {
			return "text/css";
		}
		if (extension == "js") {
			return "text/javascript";
		}
		if (extension == "json") {
			return "application/json";
		}
		if (extension == "png") {
			return "image/png";
		}
		if (extension == "jpg" || extension == "jpeg") {
			return "image/jpeg";
		}
		if (extension == "webp") {
			return "image/webp";
		}
		if (extension == "gif") {
			return "image/gif";
		}
		return "text/plain";
	}
}
