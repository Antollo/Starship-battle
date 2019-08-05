file(WRITE dummy.cpp "")
add_library(digestpp STATIC dummy.cpp)
target_include_directories(digestpp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/digestpp)