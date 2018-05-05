////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/Config.cpp
//
// summary:	Implements the configuration parser
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Config.h"
#include "Logger.h"

#include <getopt.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

namespace Draupnir
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
	Config::Config(int argc, char* const argv[])
		: m_mode(Undefined)
		, m_peer(nullptr)
		, m_isVerbose(false)
	{
		ParseCommandLine(argc, argv);
		assert(m_peer && "peer's address not specified");
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	Config::~Config()
	{
		freeaddrinfo(m_peer);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void Config::ParseCommandLine(int argc, char* const argv[])
	{
		int opt = 0;
		while ((opt = getopt(argc, argv, "c:t:vh")) != -1)
		{
			switch (static_cast<char>(opt))
			{
			case 'c':
				m_mode = Control;
				m_peer = StringToAddress(optarg);
				break;
			case 't':
				m_mode = Target;
				m_peer = StringToAddress(optarg);
				break;
			case 'v':
				m_isVerbose = true;
				Logger::GetInstance().SetVerboseMode(m_isVerbose);
				break;
			case 'h':
				ExitWithHelp();
			default:
				throw std::invalid_argument("invalid command-line parameter, run with -h for reference");
			}
		}

		if (Undefined == m_mode)
			throw std::runtime_error("either -c or -t option should be specified, run with -h for reference");
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	Config::EndPoint* Config::StringToAddress(const char* str) const
	{
		const struct addrinfo hints =
		{
			// .ai_flags - for wildcard IP address
			Target == m_mode ? AI_PASSIVE | AI_NUMERICSERV : AI_NUMERICSERV,
			// .al_family - allow IPv4 and IPv6
			AF_UNSPEC,
			// .ai_socktype - TCP socket
			SOCK_STREAM,
			// .ai_protocol = any protocol
			0,
			// .ai_addrlen
			0,
			// .ai_addr
			nullptr,
			// .ai_canonname
			nullptr,
			// .ai_next
			nullptr
		};

		std::string host(str);
		const auto portPos = host.find(':');
		if (std::string::npos == portPos || 0 == portPos)
			throw std::invalid_argument("peer address should be in form \"host:port\"");
		const std::string port = host.substr(portPos + 1);
		host.resize(portPos);

		Logger::GetInstance().Debug() << "getting information for address to "
			<< (Target == m_mode ? "listen" : "connect") << " to " << host << ':' << port;

		struct addrinfo* res = nullptr;
		int err = 0;
		if (0 != (err = getaddrinfo(host.c_str(), port.c_str(), &hints, &res)))
			throw std::runtime_error("failed to get address of " + std::string(str) + " " + gai_strerror(err));

		return res;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	[[noreturn]] void Config::ExitWithHelp() const
	{
		std::cout << "Usage: draupnir [options]\n\n"
			<< "Available options are:\n"
			<< "\t-c host:port\tstart in control mode, where host:port is the address of target\n"
			<< "\t-t [host:port]\tstart in target mode, host:port is the address to listen (default is 0.0.0.0:19680)\n"
			<< "\t-v\t\tenable verbose mode\n"
			<< "\t-h\t\tshow this message"
			<< std::endl;
		exit(EXIT_SUCCESS);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	Config::Mode Config::GetMode() const
	{
		return m_mode;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	const Config::EndPoint* Config::GetPeerAddress() const
	{
		return m_peer;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	bool Config::IsVerbose() const
	{
		return m_isVerbose;
	}
} // namespace Draupnir
