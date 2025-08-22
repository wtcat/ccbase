
target_include_directories(lv_parser
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(lv_parser
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/lv_ll.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_mem.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_mem_core_clib.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_sprintf_clib.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_string_clib.c
)