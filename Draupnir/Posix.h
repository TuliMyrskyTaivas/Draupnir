////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/Posix.h
//
// summary:	Declares the wrappers and helpers to handle POSIX API errors
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <cstring>
#include <stdexcept>

#include <unistd.h>

#define POSIX_CHECK(x) \
	for (const int _res = (x); -1 == _res;) \
	throw std::runtime_error(strerror(errno));

namespace Draupnir
{
	////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	RAII-wrapper to work with integer handles correctly. </summary>
	///
	/// <remarks>	Andrey Sploshnov, 10.03.2018. </remarks>
	///
	/// <typeparam name="T">		Generic type parameter. </typeparam>
	/// <typeparam name="TNull">	Type of the null. </typeparam>
	////////////////////////////////////////////////////////////////////////////////////
	template<typename T, T TNull = T()>
	class UniqueHandle
	{
		T m_handle ;

		public :
			UniqueHandle(std::nullptr_t = nullptr)
				: m_handle(TNull)
	{ }

	UniqueHandle(T handle)
		: m_handle(handle)
	{ }

	explicit operator bool() const
	{
		return m_handle != TNull ;
	}

	operator T&()
	{
		return m_handle ;
	}

	operator T() const
	{
		return m_handle ;
	}

	T* operator&()
	{
		return &m_handle ;
	}

	const T* operator&() const
	{
		return &m_handle ;
	}

	friend bool operator ==(UniqueHandle a, UniqueHandle b)
	{
		return a.m_handle == b.m_handle ;
	}

	friend bool operator !=(UniqueHandle a, UniqueHandle b)
	{
		return !(a == b) ;
	}

	friend bool operator ==(UniqueHandle a, std::nullptr_t)
	{
		return a.m_handle == TNull ;
	}

	friend bool operator !=(UniqueHandle a, std::nullptr_t)
	{
		return a.m_handle != TNull ;
	}

	friend bool operator ==(std::nullptr_t, UniqueHandle b)
	{
		return TNull == b.m_handle ;
	}

	friend bool operator !=(std::nullptr_t, UniqueHandle b)
	{
		return TNull != b.m_handle ;
	}
	};

	struct FDDeleter
	{
		typedef UniqueHandle<int, -1> pointer;
		void operator()(pointer p);
	};
	typedef std::unique_ptr<int, FDDeleter> SocketHandle;
} // namespace Draupnir
