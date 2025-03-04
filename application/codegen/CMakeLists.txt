
set(TARGET_NAME codegen)

add_executable(${TARGET_NAME}
    main.cc
    codegen.cc
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
    opencv_world4110d
)
