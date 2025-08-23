# xml parser library

#static_library(expat)

target_include_directories(lv_parser
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(lv_parser
	PRIVATE
	XML_STATIC=1
)

target_sources(lv_parser
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/xmlparse.c
	${CMAKE_CURRENT_SOURCE_DIR}/xmlrole.c
	${CMAKE_CURRENT_SOURCE_DIR}/xmltok.c
	${CMAKE_CURRENT_SOURCE_DIR}/xmltok_impl.c
	${CMAKE_CURRENT_SOURCE_DIR}/xmltok_ns.c
)
