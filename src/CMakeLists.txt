add_executable(starship_battle main.cpp Object.cpp Console.cpp Spaceship.cpp ContactListener.cpp CommandProcessor.cpp resourceManager.cpp)

#find_package(OpenMP REQUIRED)

target_include_directories(starship_battle PRIVATE ${SFML_INCLUDE_DIR})
target_link_libraries(starship_battle Box2D sfml-graphics sfml-window sfml-audio sfml-network sfml-system sfml-main)# OpenMP::OpenMP_CXX)
target_compile_definitions(starship_battle PRIVATE SFML_STATIC)
#set_target_properties(test PROPERTIES LINK_FLAGS_RELEASE /SUBSYSTEM:WINDOWS)