#include <memory>
#include <unordered_map>

#include "DBBase.h"
#include "DataNode/DataNodeConfig.h"
#include "FileDesc.h"
#include "Msg/MsgLogin.h"
#include "Msg/MsgLs.h"
#include "UserDesc.h"

namespace efs {
class DataNodeExecutor {
public:
    DBBase db;
    std::unordered_map<std::string, UserDesc> users;

public:
    DataNodeExecutor(const DataNodeConfig& config);
    ~DataNodeExecutor();
    DataNodeConfig config;

    std::string absolutePath(const std::string& path);
    std::string relativePath(const std::string& path);
    std::string parentPath(const std::string& path);

    ErrorCode permission(const std::string& path, int32_t uid, int32_t gid, Permission& perm);
    ErrorCode fileDesc(const std::string& path, FileDesc& fdesc);

    ErrorCode login(const std::string& user, const std::string& password);
    ErrorCode ls(const std::string& path, std::vector<FileDesc>& fs);
    ErrorCode rm(const std::string& path);
    ErrorCode chown(const std::string& path, int32_t uid, int32_t gid);
    ErrorCode chmod(const std::string& path, uint16_t mod);
    ErrorCode mkdir(const std::string& path, int32_t uid, int32_t gid);

    ErrorCode open(const std::string& path, const std::string& mod, int32_t uid, int32_t gid, FILE** fp);
    ErrorCode close(FILE** fp);
    ErrorCode write(FILE** fp, const char* buf, const int32_t& buf_size, int32_t& write_size);
    ErrorCode read(FILE** fp, char* buf, const int32_t& buf_size, int32_t& read_size);
};
}