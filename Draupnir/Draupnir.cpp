////////////////////////////////////////////////////////////////////////////////////////////////////
// file:	Draupnir.cpp
//
// summary:	entry point
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Conductor.h"
#include "Config.h"
#include "Logger.h"

#include <cstdlib>
#include <memory>

int main(int argc, char* const argv[])
try
{
	using Draupnir::Config;
	using Draupnir::Conductor;

	std::shared_ptr<Config> config = std::make_shared<Config>(argc, argv);
	std::shared_ptr<Conductor> conductor = Conductor::Create(config);

	conductor->Run();
	return EXIT_SUCCESS;
}
catch(const std::exception& e)
{
	Draupnir::Logger::GetInstance().Error() << "terminated: " << e.what();
	return EXIT_FAILURE;
}
