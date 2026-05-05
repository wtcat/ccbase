static_library(lz4)

target_sources(lz4 PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/lib/lz4.c
)