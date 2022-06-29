#include <iostream>
#include <memory>
#include <readline/readline.h>
#include <readline/history.h>
#include <iostream>

#include "ClientConfig.h"
#include "Netdisk.h"
#include "CliHandlers.h"
#include "Global.h"
#include "LineParser.h"

int main(int argc, char* argv[])
{
	std::string config_file = "ClientConfig.yaml";
	if (argc > 1) {
		config_file = std::string(argv[1]);
	}

	efs::Global::config = efs::ClientConfig(config_file);

	std::string icon =
		"        __      \n"
		"       / _|     \n"
		"   ___| |_ ___  \n"
		"  / _ \  _/ __| \n"
		" |  __/ | \__ \ \n"
		"  \___|_| |___/  Elephas File System \n"
		"\n"
		;

	std::cout << icon << std::endl;

	efs::CliHandlers::infoHandler({});

	while (1) {
		char* line = readline("> ");
		add_history(line);
		std::vector<std::string> tokens = LineParser(std::string(line)).Parse();
		free(line);

		if (tokens.size() == 0) {
			continue;
		}

		std::string cmd = tokens[0];
		if (cmd == "login") {
			efs::CliHandlers::loginHandler(tokens);
		}
		else if (cmd == "help") {
			efs::CliHandlers::helpHandler(tokens);
		}
		else if (cmd == "info") {
			efs::CliHandlers::infoHandler(tokens);
		}
		else if (cmd == "exit") {
			exit(0);
		}
		else {
			if (efs::Global::p_client == nullptr) {
				std::cout << "please login first" << std::endl;
				continue;
			}

			if (cmd == "ls") {
				efs::CliHandlers::lsHandler(tokens);
			}
			else if (cmd == "mkdir") {
				efs::CliHandlers::mkdirHandler(tokens);
			}
			else if (cmd == "rm") {
				efs::CliHandlers::rmHandler(tokens);
			}
			else if (cmd == "mv") {
				efs::CliHandlers::mvHandler(tokens);
			}
			else if (cmd == "perm") {
				efs::CliHandlers::permHandler(tokens);
			}
			else if (cmd == "cp") {
				efs::CliHandlers::cpHandler(tokens);
			}
			else if (cmd == "mount") {
				efs::CliHandlers::mountHandler(tokens);
			}
			else if (cmd == "unmount") {
				efs::CliHandlers::unmountHandler(tokens);
			}
			else if (cmd == "test") {
				efs::CliHandlers::testHandler(tokens);
			}
			else {
				std::cout << "unknown command" << std::endl;
			}
		}
	}

	return 0;
}