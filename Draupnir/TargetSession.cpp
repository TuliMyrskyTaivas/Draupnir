////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/TargetSession.cpp
//
// summary:	Implements the target session class
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TargetSession.h"
#include "TargetConductor.h"
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
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>

using namespace std::literals;

namespace Draupnir
{
////////////////////////////////////////////////////////////////////////////////////////////////////
TargetSession::TargetSession(SocketHandle&& handle, TargetConductor& parent)
	: TLSCallbacks(handle.get())
	, m_parent(parent)	
	, m_handle(std::move(handle))
	, m_sessionMgr(m_rng)
	, m_tls(*this, m_sessionMgr, m_creds, m_policy, m_rng)
{	
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TargetSession::ReceivedNetworkData(const uint8_t* const data, size_t size)
{
	m_tls.received_data(data, size);
}
	
////////////////////////////////////////////////////////////////////////////////////////////////////
void TargetSession::ReceivedConsoleData(const uint8_t* const data, size_t size)	
{
	POSIX_CHECK(write(m_ptsMaster.get(), data, size));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TargetSession::tls_session_activated()
{	
	RunShell();	
}
	
////////////////////////////////////////////////////////////////////////////////////////////////////
void TargetSession::tls_record_received(
		uint64_t seqNo __attribute__((unused)),
		const uint8_t data[],
		size_t size)
try
{
	POSIX_CHECK(write(m_ptsMaster.get(), data, size));
}
catch(const std::exception& e)
{
	ReportError(e.what());
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
	
	// Forking the child process to run the shell
	const auto forkResult = fork();
	if (-1 == forkResult)
		throw std::runtime_error("failed to run child process " + std::string(strerror(errno)));
	
	// Parent process
	if (forkResult)
	{
		// Parent process will use m_ptsMaster to communicate with the child
		m_ptsSlave.reset();
		MakeSocketNonBlocking(m_ptsMaster);
		// Add our PTY handle to the polling cycle
		m_parent.ActivateSession(*this);
	}
	// Child process
	else
	{
		m_ptsMaster.reset();
		// We're trying to set up the m_ptsSlave to be STDIN, STDOUT and STDERR
		// before exec() so everything looks normal to the shell. Unfortunately,
		// m_ptsSlave may end up as one of the STDIN_FILENO, STDOUT_FILENO or
		// STDERR_FILEN values by the chance. Set it to STDERR_FILENO + 1 to clear
		// the way for the following dup2()
		if(m_ptsSlave.get() < STDERR_FILENO + 1)
		{
			POSIX_CHECK(dup2(m_ptsSlave.get(), STDERR_FILENO + 1));
			m_ptsSlave.reset(STDERR_FILENO + 1);
		}
		
		POSIX_CHECK(dup2(m_ptsSlave.get(), STDIN_FILENO));
		POSIX_CHECK(dup2(m_ptsSlave.get(), STDOUT_FILENO));
		POSIX_CHECK(dup2(m_ptsSlave.get(), STDERR_FILENO));
		m_ptsSlave.reset();
		
		// Set the PTY as controlling
		POSIX_CHECK(setsid());
		POSIX_CHECK(ioctl(STDIN_FILENO, TIOCSCTTY, 1));
		
		// Invoke the shell		
		POSIX_CHECK(execl("/bin/sh", "/bin/sh", nullptr));
	}		
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
		m_tls.send("Error: " + message);
	
	Logger::GetInstance().Error() << message;
}
	
} // namespace Draupnir
