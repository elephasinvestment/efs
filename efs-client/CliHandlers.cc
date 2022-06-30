#include <iostream>
#include <string>
#include <format>
#include <thread>
#include <cstdlib>
#include <cstdio>

#include "CliHandlers.h"
#include "Global.h"
#include "FS.h"
#include "Error.h"

namespace efs {

	std::map<std::string, std::string> CliHandlers::helpTexts = {
		std::make_pair("login",
			"login into EFS\n"
			"Example 1: specify the login info\n"
			"login namenode_addr namenode_port username password\n"
			"\n"
			"Example 2: use the config file info\n"
			"login\n"
		),

		std::make_pair("info",
			"show info\n"
			"Example\n"
			"info\n"
		),

		std::make_pair("mount",
			"mount to local disk\n"
			"Example\n"
			"mount\n"
		),

		std::make_pair("perm",
			"modifiy file or directory permission\n"
			"Example 1: modify user permission\n"
			"perm /guest/a.txt user Tom rwx\n"
			"\n"
			"Example 2: modify group permission\n"
			"perm /guest/b.txt group StudentGroup rwx\n"
			"\n"
			"Example 3: modify directory permission recursively\n"
			"perm /guest/dira user Tom rwx r\n"
		),

		std::make_pair("chown",
			"change file or directory owner"
			"Example 1 change one file or directory owner:\n"
			"chown dir_a user_b\n"
			"\n"
			"Example 2 change directory owner recursively:\n"
			"chown dir_a user_b r\n"
		),

		std::make_pair("mkdir",
			"create a new directory\n"
			"Example:\n"
			"mkdir dira\n"
		),

		std::make_pair("rm",
			"remove file or directory\n"
			"Example:\n"
			"rm dira\n"
		),

		std::make_pair("cp",
			"copy file or directory\n"
			"Example:\n"
			"cp dir_a dir_b\n"
		),

		std::make_pair("mv",
			"rename file or directory\n"
			"Example:\n"
			"mv dir_a dir_b\n"
		),

		std::make_pair("pwd",
			"show current path\n"
			"Example:\n"
			"pwd\n"
		),

		std::make_pair("clear",
			"clear screen\n"
			"Example:\n"
			"clear\n"
		),

		std::make_pair("help",
			"show this help description\n"
			"Example:\n"
			"help\n"
		),
	};

	std::string CliHandlers::loginHandler(const std::vector<std::string>& tokens)
	{
		ErrorCode ec = ErrorCode::NONE;

		if (tokens.size() == 5) {
			Global::config.namenode_addr = tokens[1];
			Global::config.namenode_port = std::stoi(tokens[2]);
			Global::config.user = tokens[3];
			Global::config.password = tokens[4];
		}

		Global::p_client = std::make_shared<efs::Client>(Global::config);

		if ((ec = Global::p_client->connect())) {
			return CliHandlers::errorHandler(ec);
		}

		Global::p_netdisk = std::make_shared<Netdisk>(Global::p_client);
		return "";
	}

	std::string CliHandlers::lsHandler(const std::vector<std::string>& tokens)
	{
		ErrorCode ec = ErrorCode::NONE;
		std::string path = Global::pwd;
		if (tokens.size() > 1) {
			path = absolutePath(tokens[1]);
		}

		std::vector<FileDesc> fdescs;
		if ((ec = Global::p_client->ls(path, fdescs))) {
			return errorHandler(ec);
		}

		std::string res = "";
		for (const FileDesc& fdesc : fdescs) {
			res += fdesc.path + "," + Global::modToStr(fdesc.mod) + "," +
				std::to_string(fdesc.uid) + "," + std::to_string(fdesc.gid) + "," +
				std::to_string(fdesc.fsize) + "," + std::to_string(fdesc.modified_time) + "\n";
		}
		return res;
	}

	std::string CliHandlers::cdHandler(const std::vector<std::string>& tokens)
	{
		if (tokens.size() != 2) {
			return CliHandlers::wrongParas();
		}

		ErrorCode ec = ErrorCode::NONE;
		Global::pwd = absolutePath(tokens[1]);
		return "";
	}

	std::string CliHandlers::pwdHandler(const std::vector<std::string>& tokens)
	{
		return Global::pwd;
	}

	std::string CliHandlers::rmHandler(const std::vector<std::string>& tokens)
	{
		if (tokens.size() != 2) {
			return wrongParas();
		}

		ErrorCode ec = ErrorCode::NONE;
		std::string path = absolutePath(tokens[1]);
		if ((ec = Global::p_client->rmRecursive(path))) {
			return errorHandler(ec);
		}
		return "";
	}

	std::string CliHandlers::mkdirHandler(const std::vector<std::string>& tokens)
	{
		if (tokens.size() != 2) {
			return wrongParas();
		}

		ErrorCode ec = ErrorCode::NONE;
		std::string path = absolutePath(tokens[1]);
		if ((ec = Global::p_client->mkdir(path))) {
			return errorHandler(ec);
		}
		return "";
	}

	std::string CliHandlers::mvHandler(const std::vector<std::string>& tokens)
	{
		if (tokens.size() != 3) {
			return wrongParas();
		}

		ErrorCode ec = ErrorCode::NONE;
		std::string from_path = absolutePath(tokens[1]), to_path = absolutePath(tokens[2]);
		if ((ec = Global::p_client->mv(from_path, to_path))) {
			return errorHandler(ec);
		}
		return "";
	}

	std::string CliHandlers::permHandler(const std::vector<std::string>& tokens)
	{
		bool recursive = false;
		if (tokens.size() == 5 && tokens[4] == "r") {
			recursive = true;
		}
		else if (tokens.size() < 4 || tokens.size() > 5) {
			return wrongParas();
		}

		ErrorCode ec = ErrorCode::NONE;
		std::string path = absolutePath(tokens[1]);
		PermType perm_type = Global::strToPermType(tokens[2]);
		std::string name = tokens[3];
		Permission perm = Global::strToPerm(tokens[4]);

		if ((ec = Global::p_client->perm(path, name, perm_type, Permission(perm), true))) {
			return errorHandler(ec);
		}
		return "";
	}

	std::string CliHandlers::chownHandler(const std::vector<std::string>& tokens)
	{
		bool recursive = false;
		if (tokens.size() == 4 && tokens[3] == "r") {
			recursive = true;
		}
		else if (tokens.size() < 3 || tokens.size() > 4) {
			return wrongParas();
		}

		ErrorCode ec = ErrorCode::NONE;
		std::string path = absolutePath(tokens[1]);
		std::string user = tokens[2];

		if ((ec = Global::p_client->chown(path, user, recursive))) {
			return errorHandler(ec);
		}
		return "";
	}

	std::string CliHandlers::cpHandler(const std::vector<std::string>& tokens)
	{
		if (tokens.size() != 3) {
			return wrongParas();
		}

		ErrorCode ec = ErrorCode::NONE;
		std::string from_path = absolutePath(tokens[1]), to_path = absolutePath(tokens[2]);

		if ((ec = Global::p_client->cpRecursive(from_path, to_path))) {
			return errorHandler(ec);
		}

		return "";
	}

	std::string CliHandlers::mountHandler(const std::vector<std::string>& tokens)
	{
		ErrorCode ec = ErrorCode::NONE;
		if (Global::argv[0] == nullptr) {
			Global::argv[0] = new char[100];
			Global::argv[0][0] = 0;
		}

		if (Global::p_mount_thread == nullptr) {
			Global::p_mount_thread = std::make_shared<std::thread>(std::bind(&Netdisk::mount, Global::p_netdisk, Global::argc, Global::argv));
			Global::p_mount_thread->detach();
		}
		return "";
	}

	std::string CliHandlers::infoHandler(const std::vector<std::string>& tokens)
	{
		std::string res = "";
		res += "---------- account  ----------\n";
		res += " namenode_addr: " + Global::config.namenode_addr + "\n";
		res += " namenode_port: " + std::to_string(Global::config.namenode_port) + "\n";
		res += " user: " + Global::config.user + "\n";
		res += " password: " + Global::config.password + "\n";
		res += "\n";

		if (Global::p_client == nullptr) {
			return res;
		}

		res += "---------- datanode ----------\n";
		for (const HostDesc& hdesc : Global::p_client->hosts) {
			res += " " + hdesc.name + "," + hdesc.ip + "," + std::to_string(hdesc.port) + "," + std::to_string(hdesc.timestamp) + "\n";
			for (const std::string& path : hdesc.paths) {
				res += path + "\n";
			}
		}
		res += "\n";


		res += "------------ user ------------\n";
		for (const UserDesc& udesc : Global::p_client->users) {
			res += " " + udesc.user + "," + std::to_string(udesc.uid) + "," + std::to_string(udesc.gid) + "\n";
		}
		res += "\n";


		res += "----------- groups -----------\n";
		for (const GroupDesc& gdesc : Global::p_client->groups) {
			res += " " + gdesc.group + "," + std::to_string(gdesc.gid) + "\n";
		}
		res += "\n";

		return res;
	}

	std::string CliHandlers::helpHandler(const std::vector<std::string>& tokens)
	{
		std::string res;
		if (tokens.size() <= 1) {
			for (auto it = helpTexts.begin(); it != helpTexts.end(); it++) {
				res += it->first + "\n";
				res += it->second + "\n";
			}
		}
		else {
			std::string cmd = tokens[1];
			if (helpTexts.count(cmd)) {
				res += helpTexts[cmd];
			}
			else {
				res += "unknown command\n";
			}
		}
		return res;
	}

	std::string CliHandlers::clearHandler(const std::vector<std::string>& tokens)
	{
		if (system("CLS")) system("clear");
		return "";
	}

	std::string CliHandlers::absolutePath(const std::string& path)
	{
		std::vector<std::string> ps = fs::split(path);
		if (ps.size() == 0) {
			return Global::pwd;
		}

		if (ps[0] == "/") {
			return fs::format(path);
		}

		std::string cur_path = Global::pwd;
		for (std::string& p : ps) {
			if (p == ".") {
				continue;
			}
			else if (p == "..") {
				cur_path = fs::parent(cur_path);
			}
			else {
				cur_path += "/" + p;
			}
		}

		return fs::format(cur_path);
	}

	std::string CliHandlers::wrongParas()
	{
		return "wrong command\n";
	}

	std::string CliHandlers::errorHandler(efs::ErrorCode ec)
	{
		std::string res = "Error";
		res += "(" + std::to_string(int(ec)) + "): " + ErrorCodeStrMap[ec];
		res += "\n";
		return res;
	}

}
