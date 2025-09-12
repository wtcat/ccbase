
target_sources(${PROJECT_NAME}
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/plugin.cc
	${CMAKE_CURRENT_SOURCE_DIR}/../../helper/dyn_loader.cc
)

add_subdirectory(resloader)
