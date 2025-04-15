
set(TARGET_NAME rescan)

add_executable(${TARGET_NAME}
    main.cc
    rescan.cc
    doc_generator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../helper/helper.cc
)

if (CMAKE_HOST_WIN32)
target_compile_options(${TARGET_NAME}
    PRIVATE
    /wd4996
)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(LIB_OPENCV opencv_world4110d)
else ()
    set(LIB_OPENCV opencv_world4110)
endif()

collect_link_libraries(libs ${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
    ${libs}
    ${LIB_OPENCV}
    tinyxml2
)
