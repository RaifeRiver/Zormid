#include "MimeType.h"

namespace zormid::util {
	std::string getMimeType(const std::string& name) {
		if (name.ends_with(".css")) {
			return "text/css";
		}
		if (name.ends_with(".js")) {
			return "text/javascript";
		}
		if (name.ends_with(".json")) {
			return "application/json";
		}
		if (name.ends_with(".png")) {
			return "image/png";
		}
		if (name.ends_with(".jpg") || name.ends_with(".jpeg")) {
			return "image/jpeg";
		}
		if (name.ends_with(".webp")) {
			return "image/webp";
		}
		if (name.ends_with(".gif")) {
			return "image/gif";
		}
		return "text/plain";
	}
}
