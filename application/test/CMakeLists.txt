
set(TARGET_NAME test)

add_executable(${TARGET_NAME}
    test.cpp
)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(LIB_OPENCV opencv_world4110d)
else ()
    set(LIB_OPENCV opencv_world4110)
endif()

collect_link_libraries(libs ${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
    ${libs}
    ${LIB_OPENCV}
)
