////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/TargetSession.cpp
//
// summary:	Implements the target session class
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TargetSession.h"
#include "Logger.h"
#include "Posix.h"

#include <stdexcept>
#include <cstring>
#include <sstream>
#include <cerrno>
#include <chrono>
#include <thread>

#include <sys/utsname.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>

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
	// Create pipes as STDIN and STDOUT for the shell process
	int pipeHandles[2];

	POSIX_CHECK(pipe2(pipeHandles, O_NONBLOCK));
	m_pipeStdin[ReadEnd].reset(pipeHandles[ReadEnd]);
	m_pipeStdin[WriteEnd].reset(pipeHandles[WriteEnd]);

	POSIX_CHECK(pipe2(pipeHandles, O_NONBLOCK));
	m_pipeStdout[ReadEnd].reset(pipeHandles[ReadEnd]);
	m_pipeStdout[WriteEnd].reset(pipeHandles[WriteEnd]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TargetSession::ReceivedData(const uint8_t* const data, size_t size)
{
	m_tls.received_data(data, size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TargetSession::tls_session_activated()
{	
	RunShell();	
}
	
////////////////////////////////////////////////////////////////////////////////////////////////////
void TargetSession::RunShell()
try		
{		
	// Allocate PTY
	m_ptsMaster.reset(posix_openpt(O_RDWR | O_NOCTTY));
	if (!m_ptsMaster)
		throw std::runtime_error("failed to open master PT: " + std::string(strerror(errno)));
		
	POSIX_CHECK(grantpt(m_ptsMaster.get()));
	POSIX_CHECK(unlockpt(m_ptsMaster.get()));
	std::array<char, 256> masterPtsName;
	POSIX_CHECK(ptsname_r(m_ptsMaster.get(), masterPtsName.data(), masterPtsName.size()));
	
	m_ptsSlave.reset(open(masterPtsName.data(), O_RDWR | O_NOCTTY));
	if (!m_ptsSlave)
		throw std::runtime_error("failed to open slave PT: " + std::string(strerror(errno)));
	
	// Send basic information to the control node about the connecting host
	struct utsname sysInfo;
	POSIX_CHECK(uname(&sysInfo));
	
	std::ostringstream introMessage;
	introMessage << "Draupnir server version 1.0, built at \n"
		<< "Running at " << sysInfo.nodename << ' ' << sysInfo.sysname
		<< '/' << sysInfo.release << ' ' << sysInfo.machine << '\n';
	
	introMessage << "Real user: " << GetUserName(getpwuid(getuid())) << '\n';
	introMessage << "Effective user: " << GetUserName(getpwuid(geteuid())) << '\n';
	introMessage << "PTS: " << masterPtsName.data() << '\n';
	m_tls.send(introMessage.str());
	m_tls.close();
	m_handle.reset();
}
catch(const std::exception& e)
{
	ReportError(e.what());
	m_tls.close();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
std::string TargetSession::GetUserName(const struct passwd* userInfo)
{
	if (userInfo && userInfo->pw_name)
	{
		return userInfo->pw_name;
	}
	else
	{
		return "user have no name";		
	}
}
	
////////////////////////////////////////////////////////////////////////////////////////////////////
void TargetSession::ReportError(const std::string& message)
{
	if (m_tls.is_active())
		m_tls.send(message);
	
	Logger::GetInstance().Error() << message;
}
	
} // namespace Draupnir
