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

namespace Draupnir
{
	class TargetSession
	{
		TLSPolicy m_policy;
		SocketHandle m_handle;
		TLSCallbacks m_tlsCallbacks;
		CredentialsManager m_creds;
		Botan::AutoSeeded_RNG m_rng;
		Botan::TLS::Session_Manager_In_Memory m_sessionMgr;
		Botan::TLS::Server m_tls;

	public:
		explicit TargetSession(SocketHandle&& handle);

		void ReceivedData(const uint8_t* const data, size_t count);
	};
} // namespace Draupnir
