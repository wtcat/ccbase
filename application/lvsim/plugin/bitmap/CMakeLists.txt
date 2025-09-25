
add_library(bitmap SHARED )

# Bitmap font

target_include_directories(bitmap
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(bitmap
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/bitmap_font_api.c
	${CMAKE_CURRENT_SOURCE_DIR}/glyf_decompress.c
	${CMAKE_CURRENT_SOURCE_DIR}/lvgl_bitmap_font.c
	${CMAKE_CURRENT_SOURCE_DIR}/loader.cc
)

target_link_libraries(bitmap 
	base
	lvgl
	${CMAKE_CURRENT_SOURCE_DIR}/../../SDL2/lib/x64/SDL2.lib
)
