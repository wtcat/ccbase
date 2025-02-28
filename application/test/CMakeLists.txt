
set(TARGET_NAME test)

add_executable(${TARGET_NAME}
    test.cpp
)


collect_link_libraries(libs ${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
    ${libs}
    opencv_world4110d
)
