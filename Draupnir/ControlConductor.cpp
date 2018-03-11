////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/ControlConductor.cpp
//
// summary:	Implements the control conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ControlConductor.h"
#include "CredentialsManager.h"
#include "Config.h"
#include "Logger.h"

#include <cstring>
#include <cerrno>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

namespace Draupnir
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
	ControlConductor::ControlConductor(std::shared_ptr<Config> config)
		: Conductor(config)
		, m_socket(ConnectSocket())
		, m_tlsCallbacks(m_socket.get())
		, m_sessionMgr(m_rng)
		, m_tls(m_tlsCallbacks, m_sessionMgr, m_creds, m_policy, m_rng)
	{
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	SocketHandle ControlConductor::ConnectSocket()
	{
		const auto addr = GetConfig().GetPeerAddress();
		SocketHandle sock(socket(addr->ai_family, addr->ai_socktype | SOCK_NONBLOCK, 0));
		if (!sock)
			throw std::runtime_error("failed to create socket: " + std::string(strerror(errno)));

		Logger::GetInstance().Debug() << "connecting...";
		if (-1 == connect(sock.get(), addr->ai_addr, addr->ai_addrlen))
		{
			if (errno != EINPROGRESS)
				throw std::runtime_error("failed to connect: " + std::string(strerror(errno)));

			SocketHandle pollHandle(epoll_create1(0));
			if (!pollHandle)
				throw std::runtime_error("failed to poll on connect: " + std::string(strerror(errno)));

			struct epoll_event connectionEvent;
			connectionEvent.data.fd = sock.get();
			connectionEvent.events = EPOLLOUT | EPOLLIN;
			POSIX_CHECK(epoll_ctl(pollHandle.get(), EPOLL_CTL_ADD, sock.get(), &connectionEvent));

			struct epoll_event receivedEvent;
			POSIX_CHECK(epoll_wait(pollHandle.get(), &receivedEvent, 1, -1));

			int retVal = -1;
			socklen_t retValLen = sizeof(retVal);
			POSIX_CHECK(getsockopt(sock.get(), SOL_SOCKET, SO_ERROR, &retVal, &retValLen));

			if (retVal != 0)
				throw std::runtime_error("failed to connect: " + std::string(strerror(retVal)));
		}

		Logger::GetInstance().Debug() << "connection established";
		return sock;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void ControlConductor::Run()
	{
		Logger& log = Logger::GetInstance();
		log.Info() << "Draupnir is started in control mode";

		SocketHandle pollHandle(epoll_create1(0));
		if (!pollHandle)
			throw std::runtime_error("failed to poll after connect: " + std::string(strerror(errno)));

		struct epoll_event event;
		event.data.fd = m_socket.get();
		event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
		POSIX_CHECK(epoll_ctl(pollHandle.get(), EPOLL_CTL_ADD, m_socket.get(), &event));

		while (!m_tls.is_closed())
		{
			struct epoll_event incoming;
			POSIX_CHECK(epoll_wait(pollHandle.get(), &incoming, 1, -1));
			if ((incoming.events & EPOLLERR) || (incoming.events & EPOLLHUP) ||
				(!(incoming.events & EPOLLIN)))
				throw std::runtime_error("read failed");

			if (incoming.events & EPOLLRDHUP)
			{
				Logger::GetInstance().Debug() << "server terminated";
				m_socket.reset();
				break;
			}

			// We have a data on the socket waiting to be read. We must read whatever
			// data is available completely, as we are running in edge-triggered mode
			// and won't get a notification again for the same data
			while(true)
			{
				std::vector<uint8_t> buf(512);
				const ssize_t count = read(m_socket.get(), buf.data(), buf.size());
				if (count == -1)
				{
					// If errno == EAGAIN, that means we have read all the data.
					// So go back to the main loop.
					if(EAGAIN == errno)
						break;
					throw std::runtime_error("socket read error: " + std::string(strerror(errno)));
				}
				else if (count == 0)
					break;

				m_tls.received_data(buf.data(), count);
			}
		}
	}
} // namespace Draupnir
