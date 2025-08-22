# xml parser library

static_library(expat_xml)

target_include_directories(expat_xml
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(expat_xml
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/xmlparse.c
	${CMAKE_CURRENT_SOURCE_DIR}/xmlrole.c
	${CMAKE_CURRENT_SOURCE_DIR}/xmltok.c
	${CMAKE_CURRENT_SOURCE_DIR}/xmltok_impl.c
	${CMAKE_CURRENT_SOURCE_DIR}/xmltok_ns.c
)
