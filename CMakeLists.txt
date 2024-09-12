# Copyright 2022-2023 Dhiraj Wishal
# SPDX-License-Identifier: Apache-2.0

# Main build script for Xenon.
# Set the minimum required CMake version.
cmake_minimum_required(VERSION 3.22.2)

# Setup multiple configurations if it's possible.
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(IS_MULTI_CONFIG)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Profile;Release;" CACHE STRING "" FORCE)
	set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "Used by C compilers" FORCE)
	set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "Used by C++ compilers" FORCE)
	set(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "Used by compilers to generate executables" FORCE)
	set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "Used by compilers to generate shared libraries" FORCE)
	set(CMAKE_STATIC_LINKER_FLAGS_PROFILE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}" CACHE STRING "Used by compilers to generate static libraries" FORCE)
	set(CMAKE_MAP_IMPORTED_CONFIG_PROFILE Release)

endif()

# Set the basic project description.
project(
	Xenon
	VERSION 1.0.0 
	DESCRIPTION "Cross-platform graphics engine."
)

# Lets tell CMake to add the default ALL_BUILD, ZERO_CHECK and INSTALL to a group.
# This way we can make things much more simpler for Visual Studio.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "PredefinedTargets")

# Set the caches.
set(XENON_LOG_LEVEL 5 CACHE INTERNAL "This defines what to log. Checkout the wiki page for more information.")

# Add the third party libraries.
set(SPDLOG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/spdlog/include)
set(VULKAN_HEADERS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Vulkan-Headers/include)
set(VOLK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/volk)
set(VMA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/VulkanMemoryAllocator/include)
set(DIRECT_X_HEADERS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/DirectX-Headers/include)
set(D3DMA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/D3D12MemoryAllocator/include)
set(D3DMA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/D3D12MemoryAllocator/src)
set(TINYGLTF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/tinygltf)
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glm)
set(SPIRV_REFLECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/SPIRV-Reflect)
set(XXHASH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/xxHash)
set(OPTICK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/optick/src)
set(IMGUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/imgui)
set(FMT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/fmt/include)
set(ENTT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/entt/src)
set(IMGUIZMO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/ImGuizmo)
set(JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/json/include)
set(SDL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/SDL/include)

# Add the Optick library as a third party static link.
add_library(
	XenonThirdParty_Optick
	STATIC

	"${OPTICK_INCLUDE_DIR}/optick.config.h"
	"${OPTICK_INCLUDE_DIR}/optick.h"
	"${OPTICK_INCLUDE_DIR}/optick_capi.cpp"
	"${OPTICK_INCLUDE_DIR}/optick_capi.h"
	"${OPTICK_INCLUDE_DIR}/optick_common.h"
	"${OPTICK_INCLUDE_DIR}/optick_core.cpp"
	"${OPTICK_INCLUDE_DIR}/optick_core.freebsd.h"
	"${OPTICK_INCLUDE_DIR}/optick_core.h"
	"${OPTICK_INCLUDE_DIR}/optick_core.linux.h"
	"${OPTICK_INCLUDE_DIR}/optick_core.macos.h"
	"${OPTICK_INCLUDE_DIR}/optick_core.platform.h"
	"${OPTICK_INCLUDE_DIR}/optick_core.win.h"
	"${OPTICK_INCLUDE_DIR}/optick_gpu.cpp"
	"${OPTICK_INCLUDE_DIR}/optick_gpu.d3d12.cpp"
	"${OPTICK_INCLUDE_DIR}/optick_gpu.h"
	"${OPTICK_INCLUDE_DIR}/optick_gpu.vulkan.cpp"
	"${OPTICK_INCLUDE_DIR}/optick_memory.h"
	"${OPTICK_INCLUDE_DIR}/optick_message.cpp"
	"${OPTICK_INCLUDE_DIR}/optick_message.h"
	"${OPTICK_INCLUDE_DIR}/optick_miniz.cpp"
	"${OPTICK_INCLUDE_DIR}/optick_miniz.h"
	"${OPTICK_INCLUDE_DIR}/optick_serialization.cpp"
	"${OPTICK_INCLUDE_DIR}/optick_serialization.h"
	"${OPTICK_INCLUDE_DIR}/optick_server.cpp"
	"${OPTICK_INCLUDE_DIR}/optick_server.h"
)

# Set the include directories.
target_include_directories(
	XenonThirdParty_Optick 

	PRIVATE ${VULKAN_HEADERS_INCLUDE_DIR}
)

# Add the ImGui library as a third party static link.
add_library(
	XenonThirdParty_ImGui
	STATIC 

	"${IMGUI_INCLUDE_DIR}/imconfig.h"
	"${IMGUI_INCLUDE_DIR}/imgui.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui.h"
	"${IMGUI_INCLUDE_DIR}/imgui_demo.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui_draw.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui_internal.h"
	"${IMGUI_INCLUDE_DIR}/imgui_tables.cpp"
	"${IMGUI_INCLUDE_DIR}/imgui_widgets.cpp"
	"${IMGUI_INCLUDE_DIR}/imstb_rectpack.h"
	"${IMGUI_INCLUDE_DIR}/imstb_textedit.h"
	"${IMGUI_INCLUDE_DIR}/imstb_truetype.h"
)

# Set the ImGuizmo library.
add_library(
	XenonThirdParty_ImGuizmo
	STATIC 

	"${IMGUIZMO_INCLUDE_DIR}/GraphEditor.cpp"
	"${IMGUIZMO_INCLUDE_DIR}/GraphEditor.h"
	"${IMGUIZMO_INCLUDE_DIR}/ImCurveEdit.cpp"
	"${IMGUIZMO_INCLUDE_DIR}/ImCurveEdit.h"
	"${IMGUIZMO_INCLUDE_DIR}/ImGradient.cpp"
	"${IMGUIZMO_INCLUDE_DIR}/ImGradient.h"
	"${IMGUIZMO_INCLUDE_DIR}/ImGuizmo.cpp"
	"${IMGUIZMO_INCLUDE_DIR}/ImGuizmo.h"
	"${IMGUIZMO_INCLUDE_DIR}/ImSequencer.cpp"
	"${IMGUIZMO_INCLUDE_DIR}/ImSequencer.h"
	"${IMGUIZMO_INCLUDE_DIR}/ImZoomSlider.h"
)

# Set the include directories.
target_include_directories(
	XenonThirdParty_ImGuizmo 

	PRIVATE ${IMGUI_INCLUDE_DIR}
)

# Set global compile definitions.
add_compile_definitions(
	$<$<CONFIG:Debug>:XENON_DEBUG>
	$<$<CONFIG:Debug>:USE_OPTICK>
	$<$<CONFIG:Release>:XENON_RELEASE>
	$<$<CONFIG:Profile>:XENON_PROFILE>
	$<$<CONFIG:Profile>:USE_OPTICK>

	$<$<CONFIG:RelWithDebInfo>:XENON_DEBUG>
	$<$<CONFIG:RelWithDebInfo>:USE_OPTICK>
	$<$<CONFIG:MinSizeRel>:XENON_RELEASE>
	
	$<$<PLATFORM_ID:Windows>:XENON_PLATFORM_WINDOWS>
	$<$<PLATFORM_ID:Linux>:XENON_PLATFORM_LINUX>
	$<$<PLATFORM_ID:Darwin>:XENON_PLATFORM_MAC>

	XENON_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets/"
	XENON_LOG_LEVEL=${XENON_LOG_LEVEL}

	# Conditionally enable ray tracing. 
	# This is required since not all devices support ray tracing for all backends (for me Vulkan ray tracing does not work).
	# Comment this out to disable ray tracing from the studio.
	# XENON_DEV_ENABLE_RAY_TRACING

	# Conditionally enable experimental features.
	# This may contain code that might not be supported by all backends, or in other platforms, or just won't compile to begin with.
	# Comment this to disable experimental features of the engine. We recommend using this only in experimental branches.
	# XENON_ENABLE_EXPERIMENTAL
)

# If we're in a Unix operating system, find out if we're using Wayland or X11.
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	execute_process(
		COMMAND "loginctl show-session $(loginctl | grep $(whoami) | awk '{print $1}') -p Type"
		OUTPUT_VARIABLE PLATFORM_CHECK_RESULT
	)
	
	if ("${PLATFORM_CHECK_RESULT}" STREQUAL "Type=x11")
	   add_compile_definitions(XENON_PLATFORM_LINUX_X11)
	   message(STATUS "X11 detected in Linux.")
	else()
	   add_compile_definitions(XENON_PLATFORM_LINUX_WAYLAND)
	   message(STATUS "Wayland detected in Linux.")
	endif()
endif()

# Add the SDL as a subdirectory.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/SDL)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/fmt)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/spdlog)

# Include the main subdirectories.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/Xenon)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonBackend)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonCore)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonVulkanBackend)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonDX12Backend)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonPlatform)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonPlatformWindows)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonPlatformLinux)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonEvents)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonShaderBank)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Engine/XenonAssetPackager)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Studio)

# Set the output directories.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set the startup project for Visual Studio and set multi processor compilation for other projects that we build.
if (MSVC) 
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT XenonStudio)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	# Add a custom target for Visual Studio to build the solution files again.
	add_custom_target(
		XenonConfigureCMake
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ./Scripts/Bootstrap.bat
	)
	 
	# Add the custom target to a folder so it won't clutter up the solution explorer.
	set_target_properties(XenonConfigureCMake PROPERTIES FOLDER "VisualStudio")

	# Add the third party targets to a third party folder.
	set_target_properties(
		XenonThirdParty_Optick
		XenonThirdParty_ImGui
		XenonThirdParty_ImGuizmo

		SDL3_test
		SDL3-shared
		uninstall

		# fmt
		spdlog
			
		PROPERTIES FOLDER "ThirdParty"
	)

endif ()