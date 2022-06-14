#include <yaml-cpp/yaml.h>

#include "DataNodeConfig.h"
#include "Error.h"
#include "Util.h"

namespace efs {
DataNodeConfig::DataNodeConfig() { }

DataNodeConfig::DataNodeConfig(const std::string& file)
{
    YAML::Node node = YAML::LoadFile(file);

    name = node["name"].as<std::string>();
    ip = node["ip"].as<std::string>();
    port = node["port"].as<uint16_t>();
    root_path = node["root_path"].as<std::string>();
    log_path = node["log_path"].as<std::string>();
    disk_size = node["disk_size"].as<uint64_t>();
    buffer_size = node["buffer_size"].as<int32_t>();
    max_msg_size = node["max_msg_size"].as<int32_t>();

    if (!util::isPower2(buffer_size)) {
        throw ErrorCode::E_BUFFER_SIZE;
    }

    if (max_msg_size > (buffer_size >> 1)) {
        throw ErrorCode::E_BUFFER_SIZE;
    }

    name_node_ip = node["name_node_ip"].as<std::string>();
    name_node_port = node["name_node_port"].as<uint16_t>();
}

DataNodeConfig::DataNodeConfig(const DataNodeConfig& config)
{
    name = config.name;
    ip = config.ip;
    port = config.port;
    root_path = config.root_path;
    log_path = config.log_path;
    disk_size = config.disk_size;
    name_node_ip = config.name_node_ip;
    name_node_port = config.name_node_port;
}
}
