////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Droupnir/TargetConductor.h
//
// summary:	Declares the target conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Conductor.h"

namespace Draupnir
{
	class TargetConductor : public Conductor
	{
	public:
		friend class Conductor;
		virtual ~TargetConductor() = default;

		void Run() override;

	protected:
		TargetConductor(std::shared_ptr<Config> config);
		void InitializeIO() override;
		void NegotiateProtocol() override;
		void CreateTerminal();
		void ExecShell();
	};
} // namespace Draupnir
