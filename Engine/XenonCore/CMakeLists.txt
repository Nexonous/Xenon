# Copyright 2022-2023 Dhiraj Wishal
# SPDX-License-Identifier: Apache-2.0

# Set the basic project information.
project(
	XenonCore
	VERSION 1.0.0
	DESCRIPTION "The core library."
)

# Set the sources.
set(
	SOURCES

	"Common.cpp"
	"Common.hpp"
	"JobSystem.cpp"
	"JobSystem.hpp"
	"Logging.hpp"
	"SparseArray.hpp"
	"Logging.cpp"
	"XObject.cpp"
	"XObject.hpp"
	"BitSet.hpp"
	"TaskNode.cpp"
	"TaskNode.hpp"
	"TaskGraph.hpp"
	"CountingFence.cpp"
	"CountingFence.hpp"
	"GlobalConfiguration.hpp"
	"Features.hpp"
)

# Add the source group.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Add the library.
add_library(
	XenonCore
	STATIC

	${SOURCES}
)

# Set the include directories.
target_include_directories(
	XenonCore 

	PUBLIC ${SPDLOG_INCLUDE_DIR}
	PUBLIC ${FMT_INCLUDE_DIR}
	PUBLIC ${OPTICK_INCLUDE_DIR}
	PUBLIC ${JSON_INCLUDE_DIR}

	PRIVATE ${XXHASH_INCLUDE_DIR}
)

# Set the target links.
target_link_libraries(XenonCore XenonThirdParty_Optick spdlog::spdlog)

# Make sure to specify the C++ standard to C++20.
set_property(TARGET XenonCore PROPERTY CXX_STANDARD 20)

# If we are on MSVC, we can use the Multi Processor Compilation option.
if (MSVC)
	target_compile_options(XenonCore PRIVATE "/MP")	
endif ()