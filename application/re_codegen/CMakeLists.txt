
set(TARGET_NAME codegen)

add_executable(${TARGET_NAME}
    main.cc
)

collect_link_libraries(libs ${TARGET_NAME})
target_link_libraries(${TARGET_NAME}
    ${libs}
    opencv_world4110d
)
