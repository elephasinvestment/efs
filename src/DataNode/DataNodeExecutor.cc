#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include "DataNodeExecutor.h"
#include "FS.h"
#include "Util.h"

namespace efs {
DataNodeExecutor::DataNodeExecutor(const DataNodeConfig& config)
    : db(config.log_path)
{
    this->config = config;
}

DataNodeExecutor::~DataNodeExecutor()
{
}

std::string DataNodeExecutor::absolutePath(const std::string& path)
{
    auto absolute_path = std::filesystem::path(config.root_path) / path;
    return absolute_path.string();
}

std::string DataNodeExecutor::relativePath(const std::string& path)
{
    int n = path.size();
    return path.substr(config.root_path.size(), n - config.root_path.size());
}

std::string DataNodeExecutor::parentPath(const std::string& path)
{
}

ErrorCode DataNodeExecutor::fileDesc(const std::string& path, FileDesc& fdesc)
{
    std::string value;
    if (db.get(path, value)) {
        return E_DB_GET;
    }

    if (fdesc.deserialize(value.c_str(), value.size()) < 0) {
        return E_DESERIALIZE;
    }

    return ErrorCode::NONE;
}

ErrorCode DataNodeExecutor::permission(const std::string& path, int32_t uid, int32_t gid, Permission& perm)
{
    FileDesc fdesc;
    ErrorCode ec;

    if ((ec = fileDesc(path, fdesc))) {
        return ec;
    }

    if (fdesc.uid == uid) {
        perm = Permission((fdesc.mod >> 6) & 0b111);

    } else if (fdesc.gid == gid) {
        perm = Permission((fdesc.mod >> 3) & 0b111);

    } else {
        perm = Permission(fdesc.mod & 0b111);
    }

    return ErrorCode::NONE;
}

ErrorCode DataNodeExecutor::login(const std::string& user, const std::string& password)
{
    if (users.count(user) == 0) {
        return ErrorCode::E_LOGIN_USER_NOT_FOUND;

    } else if (users[user].password != password) {
        return ErrorCode::E_LOGIN_WRONG_PASSWORD;

    } else {
        return ErrorCode::NONE;
    }
}

ErrorCode DataNodeExecutor::ls(const std::string& path, std::vector<FileDesc>& fs)
{
    std::string absolute_path = absolutePath(path);
    for (const auto& entry : std::filesystem::directory_iterator(absolute_path)) {
        std::string relative_path = relativePath(entry.path());
        FileDesc fdesc;
        ErrorCode ec;
        if ((ec = fileDesc(relative_path, fdesc))) {
            return ec;
        }
        fs.push_back(fdesc);
    }

    return ErrorCode::NONE;
}

ErrorCode DataNodeExecutor::rm(const std::string& path)
{
    ErrorCode ec = ErrorCode::NONE;
    if ((ec = db.del(path))) {
        ec = ErrorCode::E_FILE_RM;
    }
    if (std::remove(path.c_str())) {
        ec = ErrorCode::E_FILE_RM;
    }
    return ec;
}

ErrorCode DataNodeExecutor::chown(const std::string& path, int32_t uid, int32_t gid)
{
    ErrorCode ec = ErrorCode::NONE;
    FileDesc fdesc;
    if ((ec = fileDesc(path, fdesc))) {
        return ec;
    }
    fdesc.uid = uid;
    fdesc.gid = gid;
    std::string value(fdesc.size(), 0);
    fdesc.serialize(value.data(), value.size());

    ec = db.put(path, value);
    return ec;
}

ErrorCode DataNodeExecutor::chmod(const std::string& path, uint16_t mod)
{
    ErrorCode ec = ErrorCode::NONE;
    FileDesc fdesc;
    if ((ec = fileDesc(path, fdesc))) {
        return ec;
    }
    fdesc.mod = mod;
    std::string value(fdesc.size(), 0);
    fdesc.serialize(value.data(), value.size());

    ec = db.put(path, value);
    return ec;
}

ErrorCode DataNodeExecutor::mkdir(const std::string& path, int32_t uid, int32_t gid)
{
    ErrorCode ec = ErrorCode::NONE;
    FileDesc fdesc;
    fdesc.path = path;
    fdesc.uid = uid;
    fdesc.gid = gid;
    fdesc.mod = 0x1111000000;

    std::string value(fdesc.size(), 0);
    fdesc.serialize(value.data(), value.size());

    if ((ec = db.put(path, value))) {
        return ec;
    }

    std::string absolute_path = absolutePath(path);
    if (!std::filesystem::create_directory(absolute_path)) {
        ec = ErrorCode::E_FILE_MKDIR;
    }
    return ec;
}

ErrorCode DataNodeExecutor::open(const std::string& path, const std::string& mod, int32_t uid, int32_t gid, FILE** fp)
{
    ErrorCode ec = ErrorCode::NONE;
    std::string parent_path = parentPath(path);
    FileDesc fdesc;

    if ((ec = fileDesc(parent_path, fdesc))) {
        return ec;
    }

    if (((*fp) = fopen(path.c_str(), mod.c_str())) == NULL) {
        return ErrorCode::E_FILE_OPEN;
    }

    if ((ec = fileDesc(path, fdesc))) {
        fdesc.path = path;
        fdesc.mod = 0b0111000000;
        fdesc.uid = uid;
        fdesc.gid = gid;
        fdesc.create_time = util::now();
        fdesc.modified_time = util::now();
        fdesc.fsize = 0;

        std::string value(fdesc.size(), 0);
        fdesc.deserialize(value.data(), value.size());

        if ((ec = db.put(path, value))) {
            fclose(*fp);
            *fp = nullptr;
            return ec;
        }
    }

    return ErrorCode::NONE;
}

ErrorCode DataNodeExecutor::close(FILE** fp)
{
    fclose(*fp);
    *fp = nullptr;
    return ErrorCode::NONE;
}

ErrorCode DataNodeExecutor::write(FILE** fp, const char* buf, const int32_t& buf_size, int32_t& write_size)
{
    write_size = std::fwrite(buf, sizeof(char), buf_size, *fp);
    if (write_size < buf_size) {
        return ErrorCode::E_FILE_WRITE;
    }
    return ErrorCode::NONE;
}

ErrorCode DataNodeExecutor::read(FILE** fp, char* buf, const int32_t& buf_size, int32_t& read_size)
{
    read_size = std::fread(buf, sizeof(char), buf_size, *fp);
    if (read_size < buf_size) {
        if (feof(*fp)) {
            return ErrorCode::E_FILE_EOF;
        }
        return ErrorCode::E_FILE_READ;
    }
    return ErrorCode::NONE;
}

}