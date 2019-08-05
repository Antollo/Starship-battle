
if(MSVC)
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src/Release)
endif()

set(SECRET $ENV{STBT_SECRET})
configure_file(secret.h.in secret.h)

add_executable(starship_battle_game main.cpp Object.cpp Console.cpp Spaceship.cpp ContactListener.cpp CommandProcessor.cpp resourceManager.cpp)

#find_package(OpenMP REQUIRED)

target_include_directories(starship_battle_game PRIVATE ${SFML_INCLUDE_DIR})
target_link_libraries(starship_battle_game Box2D sfml-graphics sfml-window sfml-audio sfml-network sfml-system nlohmann_json::nlohmann_json digestpp)# OpenMP::OpenMP_CXX)
target_compile_definitions(starship_battle_game PRIVATE SFML_STATIC)
#set_target_properties(test PROPERTIES LINK_FLAGS_RELEASE /SUBSYSTEM:WINDOWS)
