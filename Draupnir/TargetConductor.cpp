////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/TargetConductor.cpp
//
// summary:	Implements the target conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TargetConductor.h"
#include "CredentialsManager.h"
#include "Config.h"
#include "Logger.h"

#include <cstring>
#include <cerrno>
#include <vector>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

namespace Draupnir
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
	TargetConductor::TargetConductor(std::shared_ptr<Config> config)
		: Conductor(config)
		, m_listeningSocket(BindSocket())
		, m_poll(epoll_create1(0))
	{
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	SocketHandle TargetConductor::BindSocket() const
	{
		const auto addr = GetConfig().GetPeerAddress();
		SocketHandle sock(socket(addr->ai_family, addr->ai_socktype | SOCK_NONBLOCK, 0));
		if (!sock)
			throw std::runtime_error("failed to create socket: " + std::string(strerror(errno)));

		POSIX_CHECK(bind(sock.get(), addr->ai_addr, addr->ai_addrlen));
		POSIX_CHECK(listen(sock.get(), SOMAXCONN));
		return sock;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void TargetConductor::Run()
	{
		Logger& log = Logger::GetInstance();
		log.Info() << "Draupnir is started in target mode";

		{
			struct epoll_event event;
			event.data.fd = m_listeningSocket.get();
			event.events = EPOLLIN | EPOLLET;
			POSIX_CHECK(epoll_ctl(m_poll.get(), EPOLL_CTL_ADD, m_listeningSocket.get(), &event));
		}

		std::vector<struct epoll_event> events(64);
		while (true)
		{
			const int numEvents = epoll_wait(m_poll.get(), events.data(), events.size(), -1);
			POSIX_CHECK(numEvents);
			for (int idx = 0; idx < numEvents; ++idx)
			{
				const auto& event = events[idx];
				if ((event.events & EPOLLERR) || (event.events & EPOLLHUP) ||
					(!(event.events & EPOLLIN)))
				{
					log.Error() << "error reading socket " << event.data.fd;
					m_activeSessions.erase(event.data.fd);
					close(event.data.fd);
					continue;
				}

				if ((int)m_listeningSocket.get() == event.data.fd)
				{
					AcceptConnections();
				}
				else
				{
					// We have a data on the socket waiting to be read. We must read whatever
					// data is available completely, as we are running in edge-triggered mode
					// and won't get a notification again for the same data
					auto& session = m_activeSessions.at(event.data.fd);
					while (true)
					{
						std::vector<uint8_t> buf(512);
						const ssize_t count = read(event.data.fd, buf.data(), buf.size());
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

						session.ReceivedData(buf.data(), count);
					}
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void TargetConductor::AcceptConnections()
	{
		// We have notification on the listening socket, which means
		// one or more incoming connections
		while(true)
		{
			struct sockaddr inAddr;
			socklen_t inAddrLen = sizeof(struct sockaddr_in);
			SocketHandle sock(accept(m_listeningSocket.get(), &inAddr, &inAddrLen));
			if (!sock)
			{
				// We have processed all incoming connections
				if(EAGAIN == errno || EWOULDBLOCK == errno)
					break;

				throw std::runtime_error("failed to accept connection: " + std::string(strerror(errno)));
			}
			MakeSocketNonBlocking(sock);

			std::vector<char> hostname(NI_MAXHOST);
			std::vector<char> portname(NI_MAXSERV);

			const int gaiRetVal = getnameinfo(&inAddr,
				inAddrLen,
				hostname.data(),
				hostname.size(),
				portname.data(),
				portname.size(),
				NI_NUMERICHOST | NI_NUMERICSERV);
			if (0 == gaiRetVal)
			{
				Logger::GetInstance().Info() << "accepted connection from "
					<< hostname.data() << ':' << portname.data();
			}
			else
			{
				Logger::GetInstance().Error() << "failed to get peer address: "
					<< gai_strerror(gaiRetVal);
			}

			struct epoll_event event;
			event.data.fd = sock.get();
			event.events = EPOLLIN | EPOLLET;
			POSIX_CHECK(epoll_ctl(m_poll.get(), EPOLL_CTL_ADD, sock.get(), &event));

			m_activeSessions.emplace(std::make_pair(sock.get(), std::move(sock)));
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void TargetConductor::MakeSocketNonBlocking(SocketHandle& sock) const
	{
		int flags = fcntl(sock.get(), F_GETFL, 0);
		if (-1 == flags)
			throw std::runtime_error("failed to get socket attibutes: " + std::string(strerror(errno)));

		flags |= O_NONBLOCK;
		POSIX_CHECK(fcntl(sock.get(), F_SETFL, flags));
	}
} // namespace Draupnir
