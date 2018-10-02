cmake_minimum_required(VERSION 3.5.1)

############################################################################

project (Smile_Simulation)

############################################################################

# configure build locations to be: /bin & /lib on all platforms 
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Release RelWithDebInfo MinSizeRel Debug)
	set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
		"Reset the configurations"
		FORCE)
endif(CMAKE_CONFIGURATION_TYPES)

# set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
		set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} 
				${PROJECT_SOURCE_DIR}/bin )
		set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} 
				${PROJECT_SOURCE_DIR}/lib )
		set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} 
				${PROJECT_SOURCE_DIR}/lib )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# Json file of project structure for editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

###########################################################################

# add sdl2 location to environment variables for project
if( MSVC )
	set(ENV{SDL2DIR} ${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.0.7)
endif()

# includes cmake/FindSDL2.cmake and sets module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
include_directories(${SDL2_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIR})

###########################################################################

# configure a header to pass CMake settings to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/parsehead/Config.h.in"
  "${PROJECT_SOURCE_DIR}/parsehead/Config.h"
)

###########################################################################

# set compiler flags on all platforms

set(CMAKE_CXX_STANDARD 11)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wall HAS_WALL)
check_cxx_compiler_flag(-O3 HAS_OPT3)
check_cxx_compiler_flag(-O2 HAS_OPT2)
check_cxx_compiler_flag(-Os HAS_OPT0S)

if(HAS_WALL)
	if(MSVC)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -W4")
	else()
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
	endif(MSVC)
endif(HAS_WALL)

if(HAS_OPT0S)
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
endif(HAS_OPT0S)

check_cxx_compiler_flag(-g HAS_DEBUG)
check_cxx_compiler_flag(-lstdc++fs HAS_FS)

if(HAS_DEBUG)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g")
endif(HAS_DEBUG)

if(HAS_OPT3)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif(HAS_OPT3)

if(HAS_OPT2)
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2")
endif(HAS_OPT2)

# linux may require filesystem flag in C++
if( UNIX )
	set(FS_LIB "-lstdc++fs")
endif( UNIX )

###########################################################################

set( HEADERS 
	"${PROJECT_SOURCE_DIR}/parsehead/stdafx.h"
)

set( SOURCES
	"${PROJECT_SOURCE_DIR}/parsehead/glad/glad.c"
	"${PROJECT_SOURCE_DIR}/parsehead/imgui/imgui.cpp"
	"${PROJECT_SOURCE_DIR}/parsehead/imgui/imgui_draw.cpp"
	"${PROJECT_SOURCE_DIR}/parsehead/imgui/imgui_impl_sdl_gl3.cpp"
	"${PROJECT_SOURCE_DIR}/parsehead/stdafx.cpp"
	"${PROJECT_SOURCE_DIR}/parsehead/main.cpp"
)

###########################################################################

find_path(GLM_PATH glm.hpp
	PATHS
	${PROJECT_SOURCE_DIR}/parsehead/glm)
if(NOT GLM_PATH)
	message(FATAL_ERROR "GLM include not found")
endif()

# add shaders to build
file(GLOB_RECURSE SHADER_SOURCES
	LIST_DIRECTORIES false
	"${PROJECT_SOURCE_DIR}/parsehead/*.glsl*"
)

# Create executable
add_executable( smile_sim
	${SOURCES}
	${HEADERS}
	${SHADER_SOURCES}
)

target_include_directories( smile_sim PUBLIC
	"${PROJECT_SOURCE_DIR}/parsehead/imgui"
	"${PROJECT_SOURCE_DIR}/parsehead/glad"
	"${PROJECT_SOURCE_DIR}/parsehead/glm"
)

###########################################################################

if(UNIX)
	target_link_libraries( smile_sim ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} "-ldl" )
elseif(MSVC)
    target_link_libraries( smile_sim ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} )
endif(UNIX)

###########################################################################

# set visual studio startup project
set_property(DIRECTORY ${PROJECT_SOURCE_DIR}
			PROPERTY VS_STARTUP_PROJECT
			smile_sim )
