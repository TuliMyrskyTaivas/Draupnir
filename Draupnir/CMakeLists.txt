cmake_minimum_required(VERSION 3.6)
project(draupnir CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_CXX_FLAGS "-Os -ggdb -Wall -Werror")

add_executable(draupnir
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

