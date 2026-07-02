
add_subdirectory(mimalloc)
add_subdirectory(leveldb)
add_subdirectory(lz4)

add_subdirectory_ifdef(USE_SPDLOG    spdlog)
add_subdirectory_ifdef(USE_XML       tinyxml2)
add_subdirectory_ifdef(USE_IMGQUANT  libimagequant)

# LVGL
set(ROOT_DIR           ${CMAKE_CURRENT_SOURCE_DIR})
set(LVGL_ROOT_DIR      ${ROOT_DIR}/lvgl)
set(LV_BUILD_CONF_DIR  ${LVGL_ROOT_DIR} CACHE PATH " ")
include(${PROJECT_BASE}/cmake/lvgl.cmake)

