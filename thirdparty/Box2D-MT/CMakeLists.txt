file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Box2D-MT/Box2D/*.cpp)
add_library(Box2D-MT STATIC ${SOURCES})

if(MSVC)
    target_compile_options(Box2D-MT PRIVATE /FI"fix.h")
    target_include_directories(Box2D-MT PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

target_include_directories(Box2D-MT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Box2D-MT)
target_include_directories(Box2D-MT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Box2D-MT/Box2D)