set(TARGET_NAME wfc)

add_executable(${TARGET_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/cJSON.c
	${CMAKE_CURRENT_SOURCE_DIR}/wfc.c
)

target_include_directories(${TARGET_NAME}
	PRIVATE
	.
)
