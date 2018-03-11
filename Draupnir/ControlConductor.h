////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Droupnir/ControlConductor.h
//
// summary:	Declares the control conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Posix.h"
#include "Conductor.h"
#include "TLSCallbacks.h"
#include "CredentialsManager.h"

#include <botan/auto_rng.h>
#include <botan/tls_client.h>
#include <botan/tls_policy.h>
#include <botan/tls_session_manager.h>

#include <memory>

namespace Draupnir
{
	class ControlConductor final : public Conductor
	{
		SocketHandle m_socket;

		TLSCallbacks m_tlsCallbacks;
		CredentialsManager m_creds;
		Botan::AutoSeeded_RNG m_rng;
		Botan::TLS::Strict_Policy m_policy;
		Botan::TLS::Session_Manager_In_Memory m_sessionMgr;
		Botan::TLS::Client m_tls;

		SocketHandle ConnectSocket();
	public:
		void Run() final;

	protected:
		friend class Conductor;
		ControlConductor(std::shared_ptr<Config> config);
	};
} // namespace Draupnir
