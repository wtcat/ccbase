cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(app
    LANGUAGES C CXX
)

set(PROJECT_BASE ${CMAKE_CURRENT_SOURCE_DIR})

# Import user function 
include(cmake/extension.cmake)
include(cmake/extensions.cmake)

if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()


# Select operation system
# CMAKE_HOST_WIN32
# CMAKE_HOST_APPLE
# CMAKE_HOST_UNIX
message(STATUS "OS Platform: ${CMAKE_SYSTEM_NAME}")

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE : ${CMAKE_CXX_FLAGS_RELEASE}")

# Set output path for library and executable file
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})

#Options
option(USE_LEVELDB   "Enable levelDB" ON)
option(USE_SPDLOG    "Enable spdlog"  OFF)
option(USE_XML       "Enable tinyXML library" ON)
option(USE_IMGQUANT  "Enable image quant" ON)

if (USE_XML)
set(tinyxml2_SHARED_LIBS tinyxml2)
endif()

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 20)

# For lvgl
add_compile_definitions(XML_STATIC=1)

if (CMAKE_HOST_WIN32)
    add_compile_options(
        /DUNICODE=1
        /wd4251
    )
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2/include
	${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}
)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SDL2/lib/x64)

add_subdirectory(base)
add_subdirectory(thirdparty)

# Applications
add_subdirectory(application)

