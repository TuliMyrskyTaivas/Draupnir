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
		int m_socket;

		void WriteData(const uint8_t* data, size_t size);

	protected:
		explicit Conductor(std::shared_ptr<Config> config);
		const Config& GetConfig() const;

	public:
		Conductor(const Conductor&) = delete;
		Conductor& operator =(const Conductor&) = delete;
		virtual ~Conductor() = default;

		////////////////////////////////////////////////////////////////////////////
		/// <summary>	Creates an instance of Conductor requested by the current
		/// 			configuration
		/// </summary>
		///
		/// <remarks>	Andrey Sploshnov, 09.03.2018. </remarks>
		///
		/// <param name="config">	The configuration. </param>
		///
		/// <returns>	A std::shared_ptr&lt;Conductor&gt; </returns>
		////////////////////////////////////////////////////////////////////////////
		static std::shared_ptr<Conductor> Create(std::shared_ptr<Config> config);
		virtual void Run() = 0;
	};
} // namespace Draupnir
