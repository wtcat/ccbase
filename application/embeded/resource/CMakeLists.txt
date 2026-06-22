
static_library(resource)

target_sources(resource
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/resource_loader.c
	${CMAKE_CURRENT_SOURCE_DIR}/resource_lvgl.c
)
