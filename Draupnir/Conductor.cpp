////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/Conductor.cpp
//
// summary:	Implements the conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Conductor.h"
#include "ControlConductor.h"
#include "TargetConductor.h"
#include "Config.h"
#include "Logger.h"

namespace Draupnir
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
	Conductor::Conductor(std::shared_ptr<Config> config)
		: m_config(config)
		, m_socket(-1)
	{
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	const Config& Conductor::GetConfig() const
	{
		return *m_config;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<Conductor> Conductor::Create(std::shared_ptr<Config> config)
	{
		switch (config->GetMode())
		{
		case Config::Target:
			return std::shared_ptr<Conductor>(new TargetConductor(config));
		case Config::Control:
			return std::shared_ptr<Conductor>(new ControlConductor(config));
		default:
			throw std::logic_error("invalid mode");
		}
	}
} // namespace Draupnir
