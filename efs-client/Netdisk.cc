#include "Netdisk.h"
#include "compat.h"

namespace efs {

	std::shared_ptr<Client> Netdisk::p_client = nullptr;
	std::unordered_map<std::string, int32_t> Netdisk::open_fds;

	Netdisk::Netdisk(std::shared_ptr<Client> p_client)
	{
		Netdisk::p_client = p_client;
	}

	int Netdisk::mount(int argc, char *argv[])
	{
		static fuse_operations ops = {
			Netdisk::getattr,
			0, //Netdisk::readlink,
			Netdisk::mknod,
			Netdisk::mkdir,
			0, //Netdisk::unlink,
			Netdisk::rmdir,
			0, //Netdisk::symlink,
			Netdisk::rename,
			0, //Netdisk::link,
			Netdisk::chmod,
			Netdisk::chown,
			Netdisk::truncate,
			Netdisk::open,
			Netdisk::read,
			Netdisk::write,
			Netdisk::statfs,
			Netdisk::flush,
			Netdisk::release,
			0, // fsync
			0, //Netdisk::setxattr,
			0, //Netdisk::getxattr,
			0, //Netdisk::listxattr,
			0, //Netdisk::removexattr,
			Netdisk::opendir,
			Netdisk::readdir,
			Netdisk::releasedir,
			0, // fsyncdir
			Netdisk::init,
			0, // destroy
			0, // access
			0, // create
			0, // lock
			Netdisk::utimens,
			0, // bmap
		};

		return fuse_main(argc, argv, &ops, this);


	}

	Netdisk* Netdisk::getself()
	{
		return static_cast<Netdisk*>(fuse_get_context()->private_data);
	}

	fuse_timespec Netdisk::toFuseTime(int64_t t)
	{
		return fuse_timespec{
			t / 1000,
			(t % 1000) * 1000000
		};
	}

	fuse_stat Netdisk::toFuseState(const FileDesc& fdesc)
	{
		fuse_stat stat;
		stat.st_ino = 1;
		stat.st_mode = fdesc.mod;
		stat.st_nlink = 1;
		stat.st_uid = fdesc.uid;
		stat.st_gid = fdesc.gid;
		stat.st_rdev = 0;
		stat.st_ctim = toFuseTime(fdesc.create_time);
		stat.st_mtim = toFuseTime(fdesc.modified_time);
		stat.st_atim = toFuseTime(fdesc.modified_time);
		return stat;
	}

	std::shared_ptr<DataNodeConn> Netdisk::getConn(const std::string& path)
	{
		std::shared_ptr<DataNodeConn> p_conn = nullptr;
		Netdisk::p_client->getDataNodeConn(path, p_conn);
		return p_conn;
	}

	int Netdisk::getattr(const char* path, fuse_stat* stbuf, fuse_file_info* fi)
	{
		std::cout << "getatrr" << " " << path << std::endl;
		//TODO:: maybe change more elegant
		if (strcmp(path, "/") == 0) {
			stbuf->st_ino = 1;
			stbuf->st_mode = 040755;
			stbuf->st_nlink = 1;
			stbuf->st_uid = 1;
			stbuf->st_gid = 1;
			stbuf->st_rdev = 0;
			stbuf->st_ctim = toFuseTime(0);
			stbuf->st_mtim = toFuseTime(0);
			stbuf->st_atim = toFuseTime(0);
			return 0;
		}

		std::shared_ptr<DataNodeConn> p_conn = Netdisk::getConn(path);
		if (p_conn == nullptr) {
			return -ENOENT;
		}

		FileDesc fdesc;
		if (p_conn->getFileDesc(path, fdesc)) {
			return -ENOENT;
		}

		stbuf->st_ino = 1;
		stbuf->st_mode = fdesc.mod;
		stbuf->st_nlink = 1;
		stbuf->st_uid = fdesc.uid;
		stbuf->st_gid = fdesc.gid;
		stbuf->st_rdev = 0;
		stbuf->st_ctim = toFuseTime(fdesc.create_time);
		stbuf->st_mtim = toFuseTime(fdesc.modified_time);
		stbuf->st_atim = toFuseTime(fdesc.modified_time);

		return 0;
	}

	int Netdisk::readlink(const char* path, char* buf, size_t size)
	{
		std::cout << "readlink" << " "<<path << std::endl;
		return -ENOENT;
	}

	int Netdisk::mknod(const char* path, fuse_mode_t mode, fuse_dev_t dev)
	{
		std::cout << "mknod" << std::endl;
		return 0;
	}

	int Netdisk::mkdir(const char* path, fuse_mode_t mode)
	{
		std::cout << "mkdir" << std::endl;
		std::shared_ptr<DataNodeConn> p_conn = getConn(path);
		if (p_conn == nullptr) {
			return -ENOENT;
		}

		if (p_conn->mkdir(path)) {
			return -ENOENT;
		}
		return 0;
	}

	int Netdisk::unlink(const char* path)
	{
		std::cout << "unlink" << std::endl;
		return 0;
	}

	int Netdisk::rmdir(const char* path)
	{
		std::cout << "rmdir" << std::endl;
		std::shared_ptr<DataNodeConn> p_conn = getConn(path);
		if (p_conn == nullptr) {
			return -ENOENT;
		}

		if (p_conn->rm(path)) {
			return -ENOENT;
		}
		return 0;
	}

	int Netdisk::symlink(const char* dstpath, const char* srcpath)
	{
		std::cout << "symlink" << std::endl;
		return 0;
	}

	int Netdisk::rename(const char* oldpath, const char* newpath, unsigned int flags)
	{
		std::cout << "rename" << std::endl;
		return -ENOSYS;
	}

	int Netdisk::link(const char* oldpath, const char* newpath)
	{
		std::cout << "link" << std::endl;
		return 0;
	}

	int Netdisk::chmod(const char* path, fuse_mode_t mode, fuse_file_info* fi)
	{
		std::cout << "chmod" << std::endl;
		return -ENOSYS;
	}

	int Netdisk::chown(const char* path, fuse_uid_t uid, fuse_gid_t gid, fuse_file_info* fi)
	{
		std::cout << "chown" << std::endl;
		return 0;
	}

	int Netdisk::truncate(const char* path, fuse_off_t size, fuse_file_info* fi)
	{
		std::cout << "truncate" << std::endl;
		return -ENOSYS;
	}

	int Netdisk::open(const char* path, fuse_file_info* fi)
	{
		std::cout << "open" << std::endl;
		if (open_fds.count(path)) {
			return 0;
		}

		std::shared_ptr<DataNodeConn> p_conn = getConn(path);
		if (p_conn == nullptr) {
			return -ENOENT;
		}

		int32_t fd = 0;
		if (p_conn->open(path, "wr", fd)) {
			return -ENOENT;
		}

		open_fds[path] = fd;
		fi->fh = fd;

		return 0;
	}

	int Netdisk::read(const char* path, char* buf, size_t size, fuse_off_t off, fuse_file_info* fi)
	{
		std::cout << "read" << std::endl;
		if (!open_fds.count(path)) {
			return -ENOENT;
		}

		int32_t fd = open_fds[path];
		std::shared_ptr<DataNodeConn> p_conn = getConn(path);
		if (p_conn == nullptr) {
			return -ENOENT;
		}

		std::string data;
		if (p_conn->read(fd, size, data)) {
			return -ENOENT;
		}

		std::memcpy(buf, data.c_str(), data.size());
		return data.size();
	}

	int Netdisk::write(const char* path, const char* buf, size_t size, fuse_off_t off, fuse_file_info* fi)
	{
		std::cout << "write" << std::endl;
		if (!open_fds.count(path)) {
			return -ENOENT;
		}

		int32_t fd = open_fds[path];
		std::shared_ptr<DataNodeConn> p_conn = getConn(path);
		if (p_conn == nullptr) {
			return -ENOENT;
		}

		int32_t write_size = 0;
		std::string data(buf, size);
		if (p_conn->write(fd, data, write_size)) {
			return -ENOENT;
		}

		return write_size;
	}

	int Netdisk::statfs(const char* path, fuse_statvfs* stbuf)
	{
		std::cout << "statfs" << std::endl;
		std::memset(stbuf, 0, sizeof (* stbuf));
		return 0;
	}

	int Netdisk::flush(const char* path, fuse_file_info* fi)
	{
		std::cout << "flush" << std::endl;
		return -ENOSYS;
	}

	int Netdisk::release(const char* path, fuse_file_info* fi)
	{
		std::cout << "release" << std::endl;
		return 0;
	}

	int Netdisk::setxattr(const char* path, const char* name0, const char* value, size_t size, int flags)
	{
		std::cout << "setxattr" << std::endl;
		return 0;
	}

	int Netdisk::getxattr(const char* path, const char* name0, char* value, size_t size)
	{
		std::cout << "getxattr" << std::endl;
		return 0;
	}

	int Netdisk::listxattr(const char* path, char* namebuf, size_t size)
	{
		std::cout << "listxattr" << std::endl;
		return 0;
	}

	int Netdisk::removexattr(const char* path, const char* name0)
	{
		std::cout << "removexattr" << std::endl;
		return 0;
	}

	int Netdisk::opendir(const char* path, fuse_file_info* fi)
	{
		std::cout << "opendir" << std::endl;
		fi->fh = 16895;
		return 0;
	}

	void* Netdisk::init(fuse_conn_info* conn, fuse_config* conf)
	{
		std::cout << "init" << std::endl; 
		conn->want |= (conn->capable & FUSE_CAP_READDIRPLUS);
		return getself();
	}

	int Netdisk::readdir(const char* path, void* buf, 
		fuse_fill_dir_t filler, 
		fuse_off_t off, fuse_file_info* fi, enum fuse_readdir_flags)
	{
		std::cout << "======readdir" <<" "<<path<< std::endl;

		std::vector<std::shared_ptr<DataNodeConn>> p_conns;
		if (strcmp(path, "/") == 0) {
			p_client->getAllDataNodeConns(p_conns);
		}
		else {
			std::shared_ptr<DataNodeConn> p_conn = getConn(path);
			if (p_conn) {
				p_conns.push_back(p_conn);
			}
		}

		std::cout << p_conns.size() << std::endl;

		if (p_conns.size() == 0) {
			return - ENOENT;
		}

		for (auto& p_conn : p_conns) {
			std::vector<FileDesc> fdescs;
			if (p_conn->ls(path, fdescs)) {
				return -ENOENT;
			}

			for (auto& fdesc : fdescs) {

				std::cout << fdesc.path << std::endl;

				fuse_stat fstat = toFuseState(fdesc);
				if (filler(buf, fdesc.path.c_str(), &fstat, 0, FUSE_FILL_DIR_PLUS)) {
					break;
				}
			}
		}
		return 0;
	}

	int Netdisk::releasedir(const char* path, fuse_file_info* fi)
	{
		std::cout << "releasedir" << std::endl;
		return 0;
	}

	int Netdisk::utimens(const char* path, const fuse_timespec tmsp[2], fuse_file_info* fi)
	{
		std::cout << "utimens" << std::endl;
		return 0;
	}
}
