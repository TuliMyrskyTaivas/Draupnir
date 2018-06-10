cmake_minimum_required(VERSION 3.6)
project(draupnir CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compilation modes
set (CMAKE_CXX_FLAGS "-ggdb -Wall -Wpedantic -Wextra -Werror -static-libgcc -static-libstdc++")
set (CMAKE_CXX_FLAGS_DEBUG "-O0")
set (CMAKE_CXX_FLAGS_RELEASE "-Os -mtune=native -march=native")

# External modules
list (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package (Botan REQUIRED)
find_program (BotanUtil botan)
if (NOT BotanUtil)
	message (FATAL_ERROR botan utility is required to generate TLS certificate)
endif()

# Generated files
add_custom_command(OUTPUT key_obj.o
	COMMENT "Generating embedded private RSA key..."
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMAND ${BotanUtil}
	ARGS keygen --algo=RSA --output=key.rsa
	COMMAND ${CMAKE_OBJCOPY}
	ARGS --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 --rename-section .data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA key.rsa key_obj.o
)
add_custom_command(OUTPUT cert_obj.o
	COMMENT "Generating embedded self-signed X509 certificate..."
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	DEPENDS key_obj.o
	COMMAND ${BotanUtil}
	ARGS gen_self_signed key.rsa draupnir --output=cert.rsa
	COMMAND ${CMAKE_OBJCOPY}	
	ARGS --input binary --output elf64-x86-64 --binary-architecture i386:x86-64 --rename-section .data=.rodata,CONTENTS,ALLOC,LOAD,READONLY,DATA cert.rsa cert_obj.o
)
string (TIMESTAMP BUILD_DATE UTC)
configure_file (
	${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/version.h
)

include_directories(${BOTAN_INCLUDE_DIR})
add_definitions(-D_GNU_SOURCE)

add_executable (draupnir
	Conductor.h
	Conductor.cpp
	Config.h
	Config.cpp
	ControlConductor.h
	ControlConductor.cpp
	CredentialsManager.h
	CredentialsManager.cpp
	Draupnir.cpp
	Logger.h
	Logger.cpp
	Posix.h
	Posix.cpp
	TargetConductor.h
	TargetConductor.cpp
	TargetSession.h
	TargetSession.cpp
	TLSCallbacks.h
	TLSCallbacks.cpp
	TLSPolicy.h
	TLSPolicy.cpp
	key_obj.o
	cert_obj.o
)

target_link_libraries (draupnir ${BOTAN_LIBRARIES})
