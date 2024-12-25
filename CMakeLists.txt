cmake_minimum_required(VERSION 3.8)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(app
    LANGUAGES C CXX
)

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

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

include_directories(
    .
)

add_executable(${PROJECT_NAME} 
	test/test.cpp
)

add_subdirectory(base)

collect_link_libraries(libs ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}
    ${libs}
)
