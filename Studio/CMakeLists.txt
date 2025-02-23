# Copyright 2022-2023 Dhiraj Wishal
# SPDX-License-Identifier: Apache-2.0

# Set the basic project information.
project(
	XenonStudio
	VERSION 1.0.0
	DESCRIPTION "Xenon studio."
)

# Set the sources.
file(GLOB_RECURSE SHADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	*.hlsl
	*.hlsli

	*.hpp
)

set(
	SOURCES

	"Main.cpp"
	"Studio.cpp"
	"Studio.hpp"
	"UIComponent.hpp"
	"StudioConfiguration.cpp"
	"StudioConfiguration.hpp"
	"Logging.hpp"

	"Layers/ImGuiLayer.cpp"
	"Layers/ImGuiLayer.hpp"

	"UIComponents/PerformanceMetrics.cpp"
	"UIComponents/PerformanceMetrics.hpp"
	"UIComponents/LayerView.cpp"
	"UIComponents/LayerView.hpp"
	"UIComponents/PipelineEditor.cpp"
	"UIComponents/PipelineEditor.hpp"
	"UIComponents/Configuration.cpp"
	"UIComponents/Configuration.hpp"
	"UIComponents/Logs.cpp"
	"UIComponents/Logs.hpp"
	"UIComponents/PopUp.cpp"
	"UIComponents/PopUp.hpp"

	${SHADERS}
)

# Add the source group.
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Add the library.
add_executable(
	XenonStudio

	${SOURCES}
)

# Set the include directories.
target_include_directories(
	XenonStudio 

	PRIVATE ${CMAKE_SOURCE_DIR}/Engine
	PRIVATE ${IMGUI_INCLUDE_DIR}
	PRIVATE ${IMGUIZMO_INCLUDE_DIR}
)

# Add the target links.
target_link_libraries(XenonStudio XenonEngine XenonThirdParty_ImGui XenonThirdParty_ImGuizmo)

# Make sure to specify the C++ standard to C++20.
set_property(TARGET XenonStudio PROPERTY CXX_STANDARD 20)

# If we are on MSVC, we can use the Multi Processor Compilation option.
if (MSVC)
	target_compile_options(XenonStudio PRIVATE "/MP")	
endif ()

# Include the shader compile tool.
include(${CMAKE_SOURCE_DIR}/Tools/CMake/ShaderCompiler.CMake)

# Compile the shaders.
CompileShaders()

# Copy the font files to the output directory.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Fonts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

include(${CMAKE_SOURCE_DIR}/Tools/CMake/PackageAsset.CMake)

PackageAsset(XenonStudio ${CMAKE_SOURCE_DIR}/Testing/AssetPack.json ${CMAKE_CURRENT_BINARY_DIR})