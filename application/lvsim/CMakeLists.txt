cmake_minimum_required(VERSION 3.12.4)

project(lvsim)
enable_language(C CXX)

set(ROOT_DIR           ${CMAKE_CURRENT_SOURCE_DIR})
set(LVGL_ROOT_DIR      ${ROOT_DIR}/lvgl)
set(LV_BUILD_CONF_DIR  ${ROOT_DIR})

include_directories(
	${ROOT_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/SDL2/include
)
link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/SDL2/lib/x64
)

# Ignore msvc warning
if (MSVC)
	add_compile_options(
		/wd4819
		/wd4996
		/wd4018
		/wd4267
		/wd4244
	)
endif()

add_compile_definitions(
	XML_STATIC=1
)

# Set c/c++ compiler standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


add_executable(${PROJECT_NAME}
#	main.c
	main.cc
)

include(cmake/lvgl.cmake)
add_subdirectory(driver)
add_subdirectory(plugin)

target_link_libraries(${PROJECT_NAME}
	lvgl
	base
	${CMAKE_CURRENT_SOURCE_DIR}/SDL2/lib/x64/SDL2.lib
)

# Copy sdl dynamic link library
if(NOT EXISTS ${CMAKE_BINARY_DIR}/SDL2.dll)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/SDL2/lib/x64/SDL2.dll DESTINATION ${CMAKE_BINARY_DIR})
endif()