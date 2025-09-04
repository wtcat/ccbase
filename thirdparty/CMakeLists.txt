# Thirdparty

add_subdirectory(mimalloc)

add_subdirectory_ifdef(USE_LEVELDB leveldb)
add_subdirectory_ifdef(USE_SPDLOG  spdlog)
add_subdirectory_ifdef(USE_XML     tinyxml2)

