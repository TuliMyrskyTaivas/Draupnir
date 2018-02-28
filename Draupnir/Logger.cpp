////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/Logger.cpp
//
// summary:	Implements the logging facilities
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Logger.h"

#include <iostream>
#include <iomanip>
#include <cassert>
#include <chrono>
#include <ctime>
#include <mutex>

#include <sys/types.h>
#include <unistd.h>

namespace Draupnir
{
	//////////////////////////////////////////////////////////////////////////
	Logger& Logger::GetInstance()
	{
		static Logger logger;
		return logger;
	}

	//////////////////////////////////////////////////////////////////////////
	Logger::Logger()
		: m_consoleLevel(LOG_INFO)
		, m_fileLevel(LOG_NONE)
	{}

	//////////////////////////////////////////////////////////////////////////
	void Logger::EnableConsoleChannel(LogLevel level)
	{
		m_consoleLevel = level;
	}

	//////////////////////////////////////////////////////////////////////////
	void Logger::EnableFileChannel(bool overwrite, LogLevel level)
	{
		if (LOG_NONE == level)
		{
			if (m_logFile.is_open())
				m_logFile.close();
			m_fileLevel = level;
			return;
		}

		if (m_logFile.is_open())
		{
			return;
		}

		try
		{
			std::string logName;
			{
				const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
				const std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
				std::tm brokenTime;
				localtime_r(&timeNow, &brokenTime);

				std::ostringstream buf;
				char timebuf[64];
				std::strftime(&timebuf[0], sizeof(timebuf), "-%Y-%m-%dT%H%M%S", &brokenTime);
				buf << "draupnir-" << getpid() << timebuf << ".log";
				logName = std::move(buf.str());
			}

			const std::ios_base::openmode mode = std::ios::out | (overwrite ? std::ios::trunc : std::ios::app);
			m_logFile.open(logName.c_str(), mode);
			if (!m_logFile.is_open())
			{
				Error() << "failed to open log file \"" << logName << "\" for writing";
			}
			else
			{
				m_fileLevel = level;
			}
		}
		catch (const std::exception& e)
		{
			Error() << "failed to create log file: " << e.what();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void Logger::SetVerboseMode(bool isVerbose)
	{
		m_consoleLevel = LOG_DEBUG;
		if (m_logFile.is_open())
			m_fileLevel = LOG_DEBUG;
	}

	//////////////////////////////////////////////////////////////////////////
	void Logger::PutMessage(LogLevel level, const std::string& message)
	{
		static std::mutex outputLock;

		// Check if the requested log level is allowed in any of
		// configured channels
		if(level < m_consoleLevel && level < m_fileLevel)
		    return;

		// Prepare the preamble of the message
		std::string preamble;
		{
			std::ostringstream buf;
			const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			const std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
			std::tm brokenTime;
			localtime_r(&timeNow, &brokenTime);
			std::chrono::system_clock::duration milliseconds = now.time_since_epoch();
			milliseconds -= std::chrono::duration_cast<std::chrono::seconds>(milliseconds);

			buf << std::setw(4) << std::setfill('0') << brokenTime.tm_year + 1900
			    << '-' << std::setw(2) << std::setfill('0') << brokenTime.tm_mon + 1
			    << '-' << std::setw(2) << std::setfill('0') << brokenTime.tm_mday
			    << ' ' << std::setw(2) << std::setfill('0') << brokenTime.tm_hour
			    << ':' << std::setw(2) << std::setfill('0') << brokenTime.tm_min
			    << ':' << std::setw(2) << std::setfill('0') << brokenTime.tm_sec
			    << '.' << std::setw(3) << std::setfill('0') << milliseconds / std::chrono::milliseconds(1)
			    << ' ';
			preamble = std::move(buf.str());
		}

		{
			std::lock_guard<std::mutex> lock(outputLock);
			if (level >= m_consoleLevel)
			{
				std::cout << preamble << message << std::endl;
			}

			if (level >= m_fileLevel && m_logFile.good())
			{
				m_logFile << preamble << message << std::endl;
			}
		}
	}
} // namespace Draupnir
