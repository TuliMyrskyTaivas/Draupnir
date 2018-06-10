////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/TargetSession.h
//
// summary:	Declares the target session class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Posix.h"
#include "TLSPolicy.h"
#include "TLSCallbacks.h"
#include "TargetSession.h"
#include "CredentialsManager.h"

#include <botan/auto_rng.h>
#include <botan/tls_server.h>
#include <botan/tls_session_manager.h>

#include <type_traits>

struct passwd;

namespace Draupnir
{
	class TargetSession : private TLSCallbacks
	{
		TLSPolicy m_policy;
		SocketHandle m_handle;
		CredentialsManager m_creds;
		Botan::AutoSeeded_RNG m_rng;
		Botan::TLS::Session_Manager_In_Memory m_sessionMgr;
		Botan::TLS::Server m_tls;

		const int ReadEnd = 0;
		const int WriteEnd = 1;

		std::array<SocketHandle, 2> m_pipeStdin;
		std::array<SocketHandle, 2> m_pipeStdout;
		SocketHandle m_ptsMaster;
		SocketHandle m_ptsSlave;

		void tls_session_activated() final override;
		void RunShell();
		void ReportError(const std::string& message);
		std::string GetUserName(const struct passwd* userName);

	public:
		explicit TargetSession(SocketHandle&& handle);

		void ReceivedData(const uint8_t* const data, size_t count);
	};
} // namespace Draupnir
