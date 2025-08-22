set(TARGET_NAME lvgen)

include_directories(.)

add_executable(${TARGET_NAME}
    main.cc
    lvgen.cc
)

add_compile_definitions(
    LV_USE_XML=1
)

if (CMAKE_HOST_WIN32)
    target_compile_options(${TARGET_NAME} PRIVATE
        /wd4996
    )
endif()

#add_subdirectory(expat)
#add_subdirectory(parser)

collect_link_libraries(libs ${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
    ${libs}
    tinyxml2
)

# Copy sdl dynamic link library
if(NOT EXISTS ${CMAKE_BINARY_DIR}/lvdb.xml)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lvdb.xml DESTINATION ${CMAKE_BINARY_DIR})
endif()