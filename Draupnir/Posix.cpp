////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir/Posix.cpp
//
// summary:	Implements the wrappers and helpers to handle POSIX API errors
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Posix.h"

#include <unistd.h>

namespace Draupnir
{
	void FDDeleter::operator()(FDDeleter::pointer p)
	{
		close(p);
	}
} // namespace Draupnir