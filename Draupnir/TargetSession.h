////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/TargetSession.h
//
// summary:	Declares the target session class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Posix.h"
#include "TLSPolicy.h"
#include "TLSCallbacks.h"
#include "CredentialsManager.h"

#include <botan/auto_rng.h>
#include <botan/tls_server.h>
#include <botan/tls_session_manager.h>

#include <type_traits>

struct passwd;

namespace Draupnir
{

class TargetConductor;
	
class TargetSession : private TLSCallbacks
{
	TargetConductor& m_parent;	
	TLSPolicy m_policy;
	SocketHandle m_handle;
	CredentialsManager m_creds;
	Botan::AutoSeeded_RNG m_rng;
	Botan::TLS::Session_Manager_In_Memory m_sessionMgr;
	Botan::TLS::Server m_tls;		
		
	SocketHandle m_ptsMaster;
	SocketHandle m_ptsSlave;
	
	pid_t m_pid;

	// Overrides some of TLSCallbacks
	void tls_session_activated() final override;
	void tls_record_received(uint64_t seqNo, const uint8_t data[], size_t size) final override;
		
	void RunShell();
	void ReportError(const std::string& message);
	std::string GetUserName(const struct passwd* userName);

public:
	explicit TargetSession(SocketHandle&& handle, TargetConductor& parent);
	
	void OnNetworkData(const uint8_t* const data, size_t count);
	void OnConsoleData(const uint8_t* const data, size_t count);
	
	const SocketHandle& GetPtySocket() const noexcept
	{
		return m_ptsMaster;
	}
	const SocketHandle& GetNetworkSocket() const noexcept
	{
		return m_handle;
	}
	
	pid_t GetPID() const noexcept
	{
		return m_pid;
	}
};
	
} // namespace Draupnir
