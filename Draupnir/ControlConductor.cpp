////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/ControlConductor.cpp
//
// summary:	Implements the control conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ControlConductor.h"
#include "CredentialsManager.h"
#include "Config.h"
#include "Logger.h"

#include <iostream>
#include <cstring>
#include <cerrno>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

namespace Draupnir
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
	ControlConductor::ControlConductor(std::shared_ptr<Config> config)
		: Conductor(config)
		, m_socket(ConnectSocket())
		, m_sessionMgr(m_rng)
		, m_tls(*this, m_sessionMgr, m_creds, m_policy, m_rng)
	{
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	SocketHandle ControlConductor::ConnectSocket() const
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
	void ControlConductor::WriteToSocket(const std::vector<uint8_t>&& buf) const
	{
		auto size = buf.size();
		auto pos = buf.data();
		while (size)
		{
			const ssize_t res = write(m_socket.get(), pos, size);
			if (-1 == res && (errno != EINTR || errno != EAGAIN))
				throw std::runtime_error("failed to write: " + std::string(strerror(errno)));

			size -= static_cast<size_t>(res);
			pos += res;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void ControlConductor::tls_record_received(
		uint64_t seqNo __attribute__((unused)),
		const uint8_t data[],
		size_t size)
	{		
		std::cout << std::string(data, data + size);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void ControlConductor::tls_emit_data(const uint8_t data[], size_t size)
	{
		WriteToSocket(std::vector<uint8_t>(data, data + size));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void ControlConductor::tls_alert(Botan::TLS::Alert alert)
	{
		if (Botan::TLS::Alert::CLOSE_NOTIFY == alert.type())
		{
			Logger::GetInstance().Debug() << "TLS close notitification received, closing the socket";
			m_tls.close();
		}
		else
		{
			Logger::GetInstance().Error() << "TLS alert: " << alert.type_string();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	bool ControlConductor::tls_session_established(const Botan::TLS::Session& session)
	{
		const auto& info = session.server_info();
		Logger::GetInstance().Debug() << "TLS session with " << info.hostname()
		    << ":" << info.port() << " established";
		Logger::GetInstance().Debug() << session.version().to_string() << " using "
			<< session.ciphersuite().to_string();
		return true; // enable caching of the session in the configured session manager
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	void ControlConductor::tls_verify_cert_chain(
		const std::vector<Botan::X509_Certificate>& certChain __attribute__((unused)),
		const std::vector<std::shared_ptr<const Botan::OCSP::Response>>& ocspResponses __attribute__((unused)),
		const std::vector<Botan::Certificate_Store*>& trustedRoots __attribute__((unused)),
		Botan::Usage_Type usage __attribute__((unused)),
		const std::string& hostname __attribute__((unused)),
		const Botan::TLS::Policy& policy __attribute__((unused)))
	{
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
		
		// Make STDIN non blocking
		int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
		if (-1 == flags)
			throw std::runtime_error("failed to get socket attibutes: " + std::string(strerror(errno)));

		flags |= O_NONBLOCK;
		POSIX_CHECK(fcntl(STDIN_FILENO, F_SETFL, flags));
		
		// Add STDIN to the polling cycle
		event.data.fd = STDIN_FILENO;
		POSIX_CHECK(epoll_ctl(pollHandle.get(), EPOLL_CTL_ADD, STDIN_FILENO, &event));
		
		while (!m_tls.is_closed())
		{			
			std::vector<struct epoll_event> events(64);
			const int numEvents = epoll_wait(pollHandle.get(), events.data(), events.size(), -1);
			POSIX_CHECK(numEvents);
			
			for (int idx = 0; idx < numEvents; ++idx)
			{
				
				const auto& event = events[idx];
				const auto& fd = event.data.fd;
				
				if ((event.events & EPOLLERR) || (event.events & EPOLLHUP) ||
					(!(event.events & EPOLLIN)))
				{
					close(fd);
					throw std::runtime_error("read failed on FD " + std::to_string(fd));
				}					

				if (event.events & EPOLLRDHUP)
				{
					Logger::GetInstance().Debug() << "server has closed the connection";
					m_tls.close();									
				}						

				// We have a data on the socket waiting to be read. We must read whatever
				// data is available completely, as we are running in edge-triggered mode
				// and won't get a notification again for the same data
				while(true)
				{
					std::vector<uint8_t> buf(512);
					const ssize_t count = read(fd, buf.data(), buf.size());
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

					if (fd == STDIN_FILENO)
					{					
						m_tls.send(buf.data(), count);
					}
					else
					{									
						m_tls.received_data(buf.data(), count);
					}
				}
			}			
		}
	}
} // namespace Draupnir
