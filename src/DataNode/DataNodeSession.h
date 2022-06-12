#include "SessionBase.h"

namespace efs {
class DataNodeSession : public SessionBase {
public:
    std::shared_ptr<Msg> p_in_msg, p_out_msg;

public:
    DataNodeSession(int32_t buffer_size, boost::asio::ip::tcp::socket socket);

    void readHandler();
    void writeHandler();
};
}