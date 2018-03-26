////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Droupnir/TargetConductor.h
//
// summary:	Declares the target conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Posix.h"
#include "Conductor.h"
#include "TLSPolicy.h"
#include "TLSCallbacks.h"
#include "CredentialsManager.h"

#include <botan/auto_rng.h>
#include <botan/tls_server.h>
#include <botan/tls_session_manager.h>

namespace Draupnir
{
	class TargetConductor final : public Conductor
	{
		SocketHandle m_socket;
		SocketHandle m_poll;

		TLSPolicy m_policy;
		TLSCallbacks m_tlsCallbacks;
		CredentialsManager m_creds;
		Botan::AutoSeeded_RNG m_rng;
		Botan::TLS::Session_Manager_In_Memory m_sessionMgr;
		Botan::TLS::Server m_tls;

		SocketHandle BindSocket() const;
		void AcceptConnections();
		void MakeSocketNonBlocking(SocketHandle& sock) const;
	public:
		virtual ~TargetConductor() = default;
		void Run() override;

	protected:
		friend class Conductor;
		TargetConductor(std::shared_ptr<Config> config);
	};
} // namespace Draupnir
