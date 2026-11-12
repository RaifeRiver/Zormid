#include "CommandProcessor.h"

#include <fcntl.h>
#include <iostream>
#include <ranges>
#include <string>
#include <unistd.h>

#include "Zormid.h"
#include "util/StringHelper.h"

namespace zormid::command {
	static bool hasInput() {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds);
		timeval timeout = {.tv_sec = 0, .tv_usec = 0};
		return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &timeout) > 0 && FD_ISSET(STDIN_FILENO, &fds);
	}

	void processInput(Zormid& zormid) {
		std::string input;
		while (hasInput()) {
			std::cin >> input;
			if (input.empty()) {
				break;
			}
			if (std::vector<std::string> command = util::string::split(input, " "); command[0] == "exit" || command[0] == "quit") {
				zormid.stop();
			}
			else if (command[0] == "help" || command[0] == "?") {
				std::cout << "Available commands:" << std::endl;
				std::cout << "\texit, quit\t - closes Zormid server" << std::endl;
				std::cout << "\thelp, ?\t\t - displays this help message" << std::endl;
				std::cout << "\tusers\t\t - displays a list of active users" << std::endl;
			}
			else if (command[0] == "users") {
				std::unordered_map<user::manager::UserID, user::User>& users = user::manager::getLoadedUsers();
				if (users.empty()) {
					std::cout << "No active users" << std::endl;
					continue;
				}
				std::cout << "Active users:" << std::endl;
				for (user::User& user: users | std::views::values) {
					std::cout << "\t" << user.getUsername() << " - " << user.getActiveClients() << std::endl;
				}
			}
			else {
				std::cout << "Unknown command: " << command[0] << ", run help for a command list" << std::endl;
			}
		}
	}
}
