
add_library(scene SHARED )

target_include_directories(scene
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(scene
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/lvgl_res_loader.c
	${CMAKE_CURRENT_SOURCE_DIR}/res_manager_api.c
	${CMAKE_CURRENT_SOURCE_DIR}/res_mempool.c
	${CMAKE_CURRENT_SOURCE_DIR}/lz4.c
	${CMAKE_CURRENT_SOURCE_DIR}/loader.cc
)

target_link_libraries(scene 
	lvsim_interface
	base
	SDL2.lib
)
