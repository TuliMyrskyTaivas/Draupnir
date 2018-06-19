////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Droupnir/TargetConductor.h
//
// summary:	Declares the target conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Posix.h"
#include "Conductor.h"
#include "TargetSession.h"

#include <map>
#include <memory>

namespace Draupnir
{		
	class TargetConductor final : public Conductor
	{
		SocketHandle m_listeningSocket;
		SocketHandle m_poll;

		using Session = std::shared_ptr<TargetSession>;		
		std::map<int, Session> m_activeSessions;		

		SocketHandle BindSocket() const;
		void AcceptConnections();		
		
	public:
		virtual ~TargetConductor() = default;
		void Run() override;
		
		// Add PTY handle of the session to the polling cycle
		void ActivateSession(const TargetSession& session);

	protected:
		friend class Conductor;
		TargetConductor(std::shared_ptr<Config> config);
	};
} // namespace Draupnir
