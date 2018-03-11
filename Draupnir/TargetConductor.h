////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Droupnir/TargetConductor.h
//
// summary:	Declares the target conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Conductor.h"
#include <botan/tls_server.h>

namespace Draupnir
{
	class TargetConductor final : public Conductor
	{
	public:
		friend class Conductor;
		virtual ~TargetConductor() = default;

		void Run() override;

	protected:
		TargetConductor(std::shared_ptr<Config> config);
		void CreateTerminal();
		void ExecShell();
	};
} // namespace Draupnir
