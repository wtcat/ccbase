
static_library(imagequant)

target_include_directories(imagequant
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/
)

target_sources(imagequant PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/blur.c
	${CMAKE_CURRENT_SOURCE_DIR}/kmeans.c
	${CMAKE_CURRENT_SOURCE_DIR}/libimagequant.c
	${CMAKE_CURRENT_SOURCE_DIR}/mediancut.c
	${CMAKE_CURRENT_SOURCE_DIR}/mempool.c
	${CMAKE_CURRENT_SOURCE_DIR}/nearest.c
	${CMAKE_CURRENT_SOURCE_DIR}/pam.c
)