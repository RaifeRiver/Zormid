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
			std::getline(std::cin, input);
			if (input.empty()) {
				break;
			}
			if (std::vector<std::string> command = util::string::split(input, " "); command[0] == "exit" || command[0] == "quit") {
				zormid.stop();
			}
			else if (command[0] == "help" || command[0] == "?") {
				if (command.size() == 1) {
					std::cout << "Available commands:" << std::endl;
					std::cout << "\texit, quit\t - closes the Zormid server" << std::endl;
					std::cout << "\thelp, ?\t\t - displays this help message" << std::endl;
					std::cout << "\tuser" << std::endl;
					std::cout << "\t\tactive\t - displays a list of active users" << std::endl;
					std::cout << "\t\tmigrate\t - migrates user files (warning: may take a long time)" << std::endl;
					std::cout << "\tchannel" << std::endl;
					std::cout << "\t\tmigrate\t - migrates channel files (warning: may take a long time)" << std::endl;
					std::cout << "\tmigrate\t\t - migrates all files (warning: may take a long time)" << std::endl;
					std::cout << "Run help <command> for more information" << std::endl;
				}
				else if (command[1] == "exit" || command[1] == "quit") {
					std::cout << "exit, quit - closes the Zormid server" << std::endl;
				}
				else if (command[1] == "help" || command[1] == "?") {
					std::cout << "help, ?\t - displays this help message" << std::endl;
					std::cout << "\t<command> - displays help for a specific command" << std::endl;
				}
				else if (command[1] == "user") {
					std::cout << "user\t\t - manages user data and viewing user statistics" << std::endl;
					std::cout << "\tactive\t - displays a list of active (online) users and how many clients each one has" << std::endl;
					std::cout << "\tmigrate\t - migrates all user files to use the latest file format, this may take some time depending on user count during which the server will be unavailable" << std::endl;
				}
				else if (command[1] == "channel") {
					std::cout << "channel\t\t - manages channels and channel data" << std::endl;
					std::cout << "\tmigrate\t - migrates all channel files to use the latest file format, this may take some time depending on channel count during which the server will be unavailable" << std::endl;
				}
				else if (command[1] == "migrate") {
					std::cout << "migrate - migrates all user and channel files to use the latest file format, this may take some time depending on user and channel count during which the server will be unavailable" << std::endl;
				}
				else {
					std::cout << "Unknown command: " << command[1] << std::endl;
				}
			}
			else if (command[0] == "user") {
				if (command.size() < 2) {
					std::cout << "Usage: user <subcommand>" << std::endl;
					continue;
				}
				if (command[1] == "active") {
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
				else if (command[1] == "migrate") {
					std::cout << "Starting user file migration..." << std::endl;
					const uint32_t migrated = user::manager::migrateAllUsers();
					std::cout << "Migrated " << migrated << (migrated == 1? " user" : " users") << std::endl;
				}
				else {
					std::cout << "Unknown subcommand: " << command[1] << std::endl;
				}
			}
			else if (command[0] == "channel") {
				if (command.size() < 2) {
					std::cout << "Usage: channel <subcommand>" << std::endl;
					continue;
				}
				if (command[1] == "migrate") {
					std::cout << "Starting channel file migration..." << std::endl;
					const uint32_t migrated = chat::migrateAllChannels();
					std::cout << "Migrated " << migrated << (migrated == 1? " channel" : " channels") << std::endl;
				}
				else {
					std::cout << "Unknown subcommand: " << command[1] << std::endl;
				}
			}
			else if (command[0] == "migrate") {
				std::cout << "Starting file migration..." << std::endl;
				const uint32_t migratedUsers = user::manager::migrateAllUsers();
				const uint32_t migratedChannels = chat::migrateAllChannels();
				std::cout << "Migrated " << migratedUsers << (migratedUsers == 1? " user" : " users") << " and " << migratedChannels << (migratedChannels == 1? " channel" : " channels") << std::endl;
			}
			else {
				std::cout << "Unknown command: " << command[0] << ", run help for a command list" << std::endl;
			}
		}
	}
}
