#include <set>
#include <stack>
#include <tuple>
#include <cmath>

#include "Client.h"
#include "FS.h"



namespace efs {
	Client::Client(const ClientConfig& config) :
		config(config)
	{
		buffer = new char[EFS_BUFFER_SIZE];
		getDataNodes();
	}

	Client::~Client() {
		delete[] buffer;
	}

	ErrorCode Client::getDataNodes()
	{
		ErrorCode ec = ErrorCode::NONE;
		NameNodeConn conn(io_context, config.namenode_ip, config.namenode_port, "");
		if ((ec = conn.hosts(config.user, config.password, hosts, udesc))) {
			return ec;
		}

		for (auto& p : p_conns) {
			auto& p_conn = std::get<2>(p);
			if (p_conn) {
				p_conn->closeConn();
			}
		}
		p_conns.clear();

		for (int i = 0; i < hosts.size(); i++) {
			std::shared_ptr<DataNodeConn> p_conn = std::make_shared<DataNodeConn>(
				io_context,
				hosts[i].ip, hosts[i].port,
				config.user, config.password);

			if ((ec = p_conn->login())) {
				continue;
			}

			for (std::string path : hosts[i].paths) {
				int n = path.size();
				if (path[n - 1] != '/') {
					path = path + "/";
				}

				p_conns.push_back({ path, i, p_conn });
			}
		}

		std::sort(p_conns.begin(), p_conns.end(), [&](const auto& a, const auto& b) {
			return std::get<0>(a).size() > std::get<0>(b).size();
			});

		return ec;
	}

	ErrorCode Client::getDataNodeConn(const std::string& path, std::shared_ptr<DataNodeConn>& p_conn)
	{
		for (auto it = p_conns.begin(); it != p_conns.end(); it++) {
			const std::string& pre = std::get<0>(*it);
			if ((pre.size() <= path.size() && pre == path.substr(0, pre.size())) ||
				pre.size() == path.size() + 1 && pre == (path + "/")) {
				p_conn = std::get<2>(*it);
				return ErrorCode::NONE;
			}
		}
		return ErrorCode::E_NOT_FOUND;

	}

	ErrorCode Client::getAllDataNodeConns(std::vector<std::shared_ptr<DataNodeConn>>& p_conns)
	{
		std::set<std::shared_ptr<DataNodeConn>> st;
		for (auto it = this->p_conns.begin(); it != this->p_conns.end(); it++) {
			st.insert(std::get<2>(*it));
		}

		p_conns = std::vector<std::shared_ptr<DataNodeConn>>(st.begin(), st.end());
		return ErrorCode::NONE;
	}

	ErrorCode Client::getFileDesc(const std::string& path, FileDesc& fdesc)
	{
		ErrorCode ec = ErrorCode::NONE;
		std::shared_ptr<DataNodeConn> p_conn = nullptr;
		if ((ec = getDataNodeConn(path, p_conn))) {
			return ec;
		}
		
		if ((ec = p_conn->getFileDesc(path, fdesc))) {
			return ec;
		}

		return ec;
	}


	ErrorCode Client::mkdir(const std::string& path)
	{
		ErrorCode ec = ErrorCode::NONE;
		std::shared_ptr<DataNodeConn> p_conn = nullptr;

		if ((ec = getDataNodeConn(path, p_conn))) {
			return ec;
		}

		if ((ec = p_conn->mkdir(path))) {
			return ec;
		}

		return ec;
	}

	ErrorCode Client::rm(const std::string& path)
	{
		ErrorCode ec = ErrorCode::NONE;
		std::shared_ptr<DataNodeConn> p_conn = nullptr;

		if ((ec = getDataNodeConn(path, p_conn))) {
			return ec;
		}

		if ((ec = p_conn->rm(path))) {
			return ec;
		}

		return ec;
	}

	ErrorCode Client::mv(const std::string& from_path, const std::string& to_path)
	{
		ErrorCode ec = ErrorCode::NONE;
		if (from_path == to_path) {
			return ErrorCode::NONE;
		}

		std::shared_ptr<DataNodeConn> p_from_conn = nullptr;
		std::shared_ptr<DataNodeConn> p_to_conn = nullptr;
		
		if ((ec = getDataNodeConn(from_path, p_from_conn))) {
			return ec;
		}

		if ((ec = getDataNodeConn(to_path, p_to_conn))) {
			return ec;
		}

		FileDesc from_fdesc;
		if ((ec = p_from_conn->getFileDesc(from_path, from_fdesc))) {
			return ec;
		}

		// single file
		if (from_fdesc.mod & FileType::F_IFREG) {
			if (p_from_conn == p_to_conn) {
				if ((ec = p_from_conn->mv(from_path, to_path))) {
					return ec;
				}
				return ErrorCode::NONE;
			}
			else {
				if ((ec = cp(from_path, to_path, p_from_conn, p_to_conn))) {
					return ec;
				}

				if ((ec = p_from_conn->rm(from_path))) {
					return ec;
				}

				return ErrorCode::NONE;
			}
		}
		// directory
		else if (from_fdesc.mod & FileType::F_IFDIR) {
			std::stack<std::tuple<FileDesc, std::string, int8_t>> stk;
			stk.push({ from_fdesc, to_path, 0 });

			while (stk.size()) {
				auto p = stk.top();
				stk.pop();

				const FileDesc& fdesc = std::get<0>(p);
				const std::string& cur_from_path(fdesc.path);
				const std::string& cur_to_path(std::get<1>(p));
				int8_t t = std::get<2>(p);

				if (fdesc.mod & FileType::F_IFDIR) {
					if (t > 0) {
						if ((ec = p_from_conn->rm(cur_from_path))) {
							return ec;
						}
						continue;
					}

					stk.push({ fdesc, cur_from_path, 1 });

					if ((ec = p_to_conn->mkdir(cur_to_path))) {
						return ec;
					}

					std::vector<FileDesc> files;
					if ((ec = p_from_conn->ls(cur_from_path, files))) {
						return ec;
					}

					for (const FileDesc& fdesc2 : files) {
						const std::string& cur_from_path2 = fdesc2.path;
						std::string filename = fs::basename(cur_from_path2);
						std::string cur_to_path2 = fs::formatPath(cur_to_path + "/" + filename);

						stk.push({ fdesc2, cur_to_path2, 0 });
					}


				}
				else if (fdesc.mod & FileType::F_IFREG) {
					if (p_from_conn == p_to_conn) {
						if ((ec = p_from_conn->mv(cur_from_path, cur_to_path))) {
							return ec;
						}
					}
					else {
						if ((ec = cp(cur_from_path.c_str(), cur_to_path.c_str(), p_from_conn, p_to_conn))) {
							return ec;
						}

						if ((ec = p_from_conn->rm(cur_from_path))) {
							return ec;
						}
					}
				}
				else {
					return E_FILE_UNKNOWN_TYPE;
				}
			}

			return ErrorCode::NONE;
		}
		else {
			return E_FILE_UNKNOWN_TYPE;
		}
	}


	ErrorCode Client::ls(const std::string& path, std::vector<FileDesc>& fdescs)
	{
		ErrorCode ec = ErrorCode::NONE;
		std::vector<std::shared_ptr<DataNodeConn>> p_conns;
		if (path == "/") {
			if ((ec = getAllDataNodeConns(p_conns))) {
				return ec;
			}
		}
		else {
			std::shared_ptr<DataNodeConn> p_conn = nullptr;
			if ((ec = getDataNodeConn(path, p_conn))) {
				return ec;
			}
			p_conns.push_back(p_conn);
		}

		if (p_conns.size() == 0) {
			return E_FILE_NOT_FOUND;
		}

		for (auto& p_conn : p_conns) {
			std::vector<FileDesc> cur_fdescs;
			if ((ec = p_conn->ls(path, cur_fdescs))) {
				return ec;
			}

			for (auto& fdesc : cur_fdescs) {
				fdescs.push_back(fdesc);
			}
		}

		return ErrorCode::NONE;
	}

	ErrorCode Client::cp(const std::string& from_path, const std::string& to_path, std::shared_ptr<DataNodeConn> p_from_conn, std::shared_ptr<DataNodeConn> p_to_conn)
	{
		ErrorCode ec = ErrorCode::NONE;
		int64_t offset = 0;
		while (!ec) {
			int32_t rs = 0;
			ec = p_from_conn->readOffset(from_path, EFS_MAX_READ_SIZE, offset, buffer, rs);
			if ((ec && ec != ErrorCode::E_FILE_EOF) || rs < 0) {
				return ec;
			}

			int32_t ws = 0;
			ec = p_to_conn->writeOffset(to_path, buffer, rs, offset, ws);
			if (ec || ws != rs) {
				return ec;
			}

			offset += ws;
		}

		return ErrorCode::NONE;
	}

	ErrorCode Client::openOffset(const std::string& path)
	{
		ErrorCode ec = ErrorCode::NONE;
		std::shared_ptr<DataNodeConn> p_conn = nullptr;

		if ((ec = getDataNodeConn(path, p_conn))) {
			return ec;
		}

		if ((ec = p_conn->openOffset(path))) {
			return ec;
		}

		return ErrorCode::NONE;
	}

	ErrorCode Client::readOffset(const std::string& path, const int64_t& read_size, const int64_t& offset, char* data, int64_t& real_read_size)
	{
		ErrorCode ec = ErrorCode::NONE;
		std::shared_ptr<DataNodeConn> p_conn = nullptr;
		if ((ec = getDataNodeConn(path, p_conn))) {
			return ec;
		}

		int64_t i = 0;
		while (i < read_size) {
			int64_t rs = std::min(read_size - i, int64_t(EFS_MAX_READ_SIZE));
			int32_t cur_real_read_size = 0;
			ErrorCode ec = p_conn->readOffset(path, rs, offset + i, data + i, cur_real_read_size);
			if (ec && ec != ErrorCode::E_FILE_EOF) {
				return ec;
			}

			i += cur_real_read_size;
			real_read_size = i;
			if (ec == ErrorCode::E_FILE_EOF) {
				break;
			}
		}

		return ErrorCode::NONE;
	}
	ErrorCode Client::writeOffset(const std::string& path, const char* data, int64_t write_size, const int64_t& offset, int64_t& real_write_size)
	{
		ErrorCode ec = ErrorCode::NONE;
		std::shared_ptr<DataNodeConn> p_conn = nullptr;
		if ((ec = getDataNodeConn(path, p_conn))) {
			return ec;
		}
		
		int64_t i = 0;
		while (i < write_size) {
			int32_t ws = std::min(write_size - i, int64_t(EFS_MAX_READ_SIZE));
			int32_t cur_real_write_size = 0;
			if ((ec = p_conn->writeOffset(path, data + i, ws, offset + i, cur_real_write_size))) {
				return ec;
			}
			i += cur_real_write_size;
			real_write_size = i;
		}

		return ErrorCode::NONE;
	}

	ErrorCode Client::truncate(const std::string& path, const int64_t offset)
	{
		ErrorCode ec = ErrorCode::NONE;
		std::shared_ptr<DataNodeConn> p_conn = nullptr;
		if ((ec = getDataNodeConn(path, p_conn))) {
			return ec;
		}

		if ((ec = p_conn->truncate(path, offset))) {
			return ec;
		}

		return ErrorCode::NONE;
	}
}