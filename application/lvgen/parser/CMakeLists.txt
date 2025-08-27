# lvgl parser library

static_library(lv_parser)

target_include_directories(lv_parser
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(lv_parser
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/lv_xml.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_xml_component.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_xml_parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_xml_style.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_xml_utils.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_xml_widget.c
	${CMAKE_CURRENT_SOURCE_DIR}/lv_xml_base_types.c

	${CMAKE_CURRENT_SOURCE_DIR}/parsers/lv_xml_obj_parser.c
	${CMAKE_CURRENT_SOURCE_DIR}/parsers/lv_xml_label_parser.c

)

add_subdirectory(lib)
add_subdirectory(expat)