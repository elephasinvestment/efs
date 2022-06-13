#include <string>

#include "Msg/Msg.h"

namespace efs {
struct MsgClose : Msg {
    int64_t fd;

    MsgClose()
    {
        fd = -1;
    }
};

struct MsgCloseResp : Msg {

    MsgCloseResp()
    {
    }
};
}