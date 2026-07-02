cmake_minimum_required(VERSION 3.12.4)

project(lvsim)
enable_language(C CXX)

add_library(lvsim_interface INTERFACE )
target_include_directories(lvsim_interface INTERFACE 
	${CMAKE_CURRENT_SOURCE_DIR}
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

# Set c/c++ compiler standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


add_executable(${PROJECT_NAME}
	main.c
#	main.cc
)



add_subdirectory(driver)
add_subdirectory(plugin)

target_link_libraries(${PROJECT_NAME}
	lvsim_interface
	lvgl
	base
	embeded
	resource
	SDL2.lib
)

# Copy sdl dynamic link library
if(NOT EXISTS ${CMAKE_BINARY_DIR}/SDL2.dll)
	file(COPY ${PROJECT_BASE}/thirdparty/SDL2/lib/x64/SDL2.dll DESTINATION ${CMAKE_BINARY_DIR})
endif()