
set(TARGET_NAME rescan)

add_executable(${TARGET_NAME}
    main.cc
    rescan.cc
    xml_generator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../helper/helper.cc
)

if (CMAKE_HOST_WIN32)
target_compile_options(${TARGET_NAME}
    PRIVATE
    /wd4996
)
endif()

collect_link_libraries(libs ${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
    ${libs}
    tinyxml2
    opencv_world4110d
)
