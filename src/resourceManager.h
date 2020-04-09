#ifndef RESOURSEMANAGER_H_
#define RESOURSEMANAGER_H_

#include <map>
#include <array>
#include <fstream>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <digestpp.hpp>
#include "Vec2f.h"

namespace resourceManager
{
    void playSound(const std::string& name);
    const sf::SoundBuffer &getSoundBuffer(const std::string &name);
    const sf::Font& getFont(const std::string& name);
    const json& getJSON(const std::string& name);
}
#endif /* !RESOURSEMANAGER_H_ */
