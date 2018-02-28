cmake_minimum_required(VERSION 3.6)
project(draupnir CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_CXX_FLAGS "-Os -ggdb -Wall -Werror")

list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package (Botan REQUIRED)

add_executable (draupnir
	Conductor.h
	Conductor.cpp
	Config.h
	Config.cpp
	ControlConductor.h
	ControlConductor.cpp
	Draupnir.cpp
	Logger.h
	Logger.cpp
	TargetConductor.h
	TargetConductor.cpp
)

target_link_libraries (draupnir ${BOTAN_LIBRARIES})
set_target_properties (draupnir PROPERTIES
    INCLUDE_DIRECTORIES ${BOTAN_INCLUDE_DIR}
)
