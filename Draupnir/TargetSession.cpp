////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/TargetSession.cpp
//
// summary:	Implements the target session class
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TargetSession.h"
#include "Logger.h"
#include "Posix.h"

#include <sstream>
#include <chrono>
#include <thread>

#include <sys/utsname.h>

using namespace std::literals;

namespace Draupnir
{
////////////////////////////////////////////////////////////////////////////////////////////////////
TargetSession::TargetSession(SocketHandle&& handle)
	: TLSCallbacks(handle.get())
	, m_handle(std::move(handle))
	, m_sessionMgr(m_rng)
	, m_tls(*this, m_sessionMgr, m_creds, m_policy, m_rng)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TargetSession::ReceivedData(const uint8_t* const data, size_t size)
{
	m_tls.received_data(data, size);
}
} // namespace Draupnir

////////////////////////////////////////////////////////////////////////////////////////////////////
void Draupnir::TargetSession::tls_session_activated()
{
	struct utsname sysInfo;
	POSIX_CHECK(uname(&sysInfo));

	std::ostringstream introMessage;
	introMessage << "Draupnir server version 1.0, built at \n"
		<< "Running at " << sysInfo.nodename << ' ' << sysInfo.sysname
		<< '/' << sysInfo.release << ' ' << sysInfo.machine << '\n';

	m_tls.send(introMessage.str());
	m_tls.close();
}
