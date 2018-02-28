////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/Conductor.h
//
// summary:	Declares the conductor class
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <memory>

namespace Draupnir
{
	class Config;

	class Conductor
	{
		std::shared_ptr<Config> m_config;
	protected:
		explicit Conductor(std::shared_ptr<Config> config);
		const Config& GetConfig() const;

		virtual void InitializeIO() = 0;
		virtual void NegotiateProtocol() = 0;

	public:
		Conductor(const Conductor&) = delete;
		Conductor& operator =(const Conductor&) = delete;
		virtual ~Conductor() = default;

		static std::shared_ptr<Conductor> Create(std::shared_ptr<Config> config);
		virtual void Run();
	};
} // namespace Draupnir
