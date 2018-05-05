////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/config.h
//
// summary:	Declares the configuration parser
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace Draupnir
{
	class Config
	{
		typedef struct addrinfo EndPoint;

		void ParseCommandLine(int argc, char* const argv[]);
		EndPoint* StringToAddress(const char* str) const;
		[[noreturn]] void ExitWithHelp() const;

	public:
		enum Mode
		{
			Undefined,
			Target,
			Control
		};

		explicit Config(int argc, char* const argv[]);
		~Config();

		Mode GetMode() const;
		const EndPoint* GetPeerAddress() const;
		bool IsVerbose() const;

	private:
		Mode m_mode;
		EndPoint* m_peer;
		bool m_isVerbose;
	};
} // namespace Draupnir
