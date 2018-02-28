////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Droupnir/ControlConductor.h
//
// summary:	Declares the control conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Conductor.h"

namespace Draupnir
{
	class ControlConductor : public Conductor
	{
	public:
		friend class Conductor;

		virtual void Run() override;

	protected:
		ControlConductor(std::shared_ptr<Config> config);
		void InitializeIO() override;
		void NegotiateProtocol() override;
	};
} // namespace Draupnir
