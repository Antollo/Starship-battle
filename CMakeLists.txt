cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(Starship_battle)

if(MSVC)
    add_compile_options(/W3 /MT /MP)
    add_compile_options(/O2 /GS- /GL)
    add_link_options(/LTCG)
else()
    add_compile_options(-O3)
    link_libraries(stdc++fs)
endif()

add_subdirectory(thirdparty/SFML)
add_subdirectory(thirdparty/Box2D-MT)
add_subdirectory(thirdparty/json)
add_subdirectory(thirdparty/digestpp)
add_subdirectory(src)
