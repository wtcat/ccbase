
set(TARGET_NAME strgen)
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/strings.xls)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/strings.xls DESTINATION ${PROJECT_BINARY_DIR})
endif()

# pugixml: single translation unit
add_library(pugixml STATIC 
	${THIRD_PARTY_DIR}/pugixml/pugixml.cpp
)
target_include_directories(pugixml PUBLIC ${THIRD_PARTY_DIR}/pugixml)

# miniz: split sources; miniz_export.h is vendored (CMake normally gens it)
add_library(miniz STATIC
  ${THIRD_PARTY_DIR}/miniz/miniz.c
  ${THIRD_PARTY_DIR}/miniz/miniz_tdef.c
  ${THIRD_PARTY_DIR}/miniz/miniz_tinfl.c
  ${THIRD_PARTY_DIR}/miniz/miniz_zip.c)
target_include_directories(miniz PUBLIC ${THIRD_PARTY_DIR}/miniz)

# libxls: compile the C sources directly with a generated config.h
set(LIBXLS_CONFIG_DIR ${CMAKE_BINARY_DIR}/libxls_config)
if (CMAKE_HOST_WIN32)
file(WRITE ${LIBXLS_CONFIG_DIR}/config.h
  "#pragma once\n"
  "#define PACKAGE_VERSION \"1.6.3\"\n"
  "#define ICONV_CONST\n")  # glibc iconv takes char** (non-const) -> empty
else ()
file(WRITE ${LIBXLS_CONFIG_DIR}/config.h
  "#pragma once\n"
  "#define PACKAGE_VERSION \"1.6.3\"\n"
  "#define HAVE_ICONV 1\n"
  "#define ICONV_CONST\n")  # glibc iconv takes char** (non-const) -> empty
endif()
add_library(xls STATIC
  ${THIRD_PARTY_DIR}/libxls/src/xls.c
  ${THIRD_PARTY_DIR}/libxls/src/ole.c
  ${THIRD_PARTY_DIR}/libxls/src/xlstool.c
  ${THIRD_PARTY_DIR}/libxls/src/endian.c
  ${THIRD_PARTY_DIR}/libxls/src/locale.c
)
target_include_directories(xls
  PUBLIC  ${THIRD_PARTY_DIR}/libxls/include
  PRIVATE ${LIBXLS_CONFIG_DIR}
)

add_executable(${TARGET_NAME}
  main.cc
  cli.cc
  bin_writer.cc
  packer.cc
  ids_header.cc
  excel_reader.cc
  xls_reader.cc
  xlsx_reader.cc
)

target_include_directories(${TARGET_NAME}
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

if (CMAKE_HOST_WIN32)
target_compile_options(${TARGET_NAME}
    PRIVATE
    /wd4996
    /wd4200
)
endif()

target_link_libraries(${TARGET_NAME}
	xls 
	miniz 
	pugixml
)
