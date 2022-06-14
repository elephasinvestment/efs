#include <string>

#include "Serialize.h"
#include "Util.h"

namespace efs {

enum Permission : uint8_t {
    NONE = 0x0,
    R = 0x1,
    W = 0x2,
    X = 0x4,
};

struct FileDesc {
    std::string path;
    int64_t fsize;

    int32_t uid;
    int32_t gid;
    uint16_t mod;
    int64_t create_time;
    int64_t modified_time;

    FileDesc();
    FileDesc(const FileDesc& fdesc);
    FileDesc(FileDesc&& fdesc);
    FileDesc& operator=(const FileDesc& fdesc);
    Permission perm(int32_t uid, int64_t gid) const;

    inline int32_t size() const
    {
        int32_t res = 0;
        res += serialize::size(path);
        res += serialize::size(fsize);
        res += serialize::size(uid);
        res += serialize::size(gid);
        res += serialize::size(mod);
        res += serialize::size(create_time);
        res += serialize::size(modified_time);
        return res;
    }

    inline int32_t serialize(char* buf, int32_t buf_size) const
    {
        if (this->size() > buf_size) {
            return -1;
        }
        int32_t size = 0;
        size += serialize::serialize(path, buf + size, buf_size - size);
        size += serialize::serialize(fsize, buf + size, buf_size - size);
        size += serialize::serialize(uid, buf + size, buf_size - size);
        size += serialize::serialize(gid, buf + size, buf_size - size);
        size += serialize::serialize(mod, buf + size, buf_size - size);
        size += serialize::serialize(create_time, buf + size, buf_size - size);
        size += serialize::serialize(modified_time, buf + size, buf_size - size);
        return size;
    }

    inline int32_t deserialize(const char* buf, int32_t buf_size)
    {
        int size = 0, size1 = 0;

        if ((size1 = serialize::deserialize(path, buf + size, buf_size - size)) < 0) {
            return -1;
        }
        size += size1;

        if ((size1 = serialize::deserialize(fsize, buf + size, buf_size - size)) < 0) {
            return -1;
        }
        size += size1;

        if ((size1 = serialize::deserialize(uid, buf + size, buf_size - size)) < 0) {
            return -1;
        }
        size += size1;

        if ((size1 = serialize::deserialize(gid, buf + size, buf_size - size)) < 0) {
            return -1;
        }
        size += size1;

        if ((size1 = serialize::deserialize(mod, buf + size, buf_size - size)) < 0) {
            return -1;
        }
        size += size1;

        if ((size1 = serialize::deserialize(create_time, buf + size, buf_size - size)) < 0) {
            return -1;
        }
        size += size1;

        if ((size1 = serialize::deserialize(modified_time, buf + size, buf_size - size)) < 0) {
            return -1;
        }
        size += size1;

        return size;
    }
};
}