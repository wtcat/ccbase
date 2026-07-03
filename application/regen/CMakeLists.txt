
set(TARGET_NAME regen)

if(NOT EXISTS ${PROJECT_BINARY_DIR}/IMG)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/IMG DESTINATION ${PROJECT_BINARY_DIR})
endif()

add_executable(${TARGET_NAME}
    main.cc
    regen.cc
    reconv.cc
	quantizer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../helper/utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../helper/fnmatch.cc
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

collect_link_libraries(libs ${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
    ${libs}
)
