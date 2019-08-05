cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(Starship_battle)

if(MSVC)
    add_compile_options(/O2 /W3 /MT)
    #/GL
else()
    if(NOT CMAKE_BUILD_TYPE)
        add_compile_options(-O3)
    endif()
endif()

add_subdirectory(thirdparty/SFML)
add_subdirectory(thirdparty/Box2D)
add_subdirectory(thirdparty/json)
add_subdirectory(thirdparty/digestpp)
add_subdirectory(src)
