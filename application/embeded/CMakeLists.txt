
static_library(embeded)
target_sources(embeded
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/wf_loader.c
)

add_subdirectory(decoder)
add_subdirectory(widget)
add_subdirectory(resource)