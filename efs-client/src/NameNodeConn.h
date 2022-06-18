#pragma once

#include <string>
#include <stdint.h>
#include <boost/asio.hpp>

#include "Conn.h"
#include "Error.h"
#include "Msg/NameNode/MsgAccount.h"
#include "Msg/NameNode/MsgHost.h"

namespace efs {

	class NameNodeConn : public Conn {
	public:
		std::string ip;
		uint16_t port;
		std::string token;

	public:
		NameNodeConn(boost::asio::io_context& io_context,
			const std::string& ip, const uint16_t& port, 
			const std::string& token);

		ErrorCode host(MsgHostResp& m_host_resp);
	};

}

