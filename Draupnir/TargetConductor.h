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

namespace Draupnir
{
	class TargetConductor final : public Conductor
	{
		SocketHandle m_listeningSocket;
		SocketHandle m_poll;

		std::map<int, TargetSession> m_activeSessions;

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
