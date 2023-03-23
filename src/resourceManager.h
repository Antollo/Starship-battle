#ifndef RESOURSEMANAGER_H_
#define RESOURSEMANAGER_H_

#include <string>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include "Vec2f.h"

class TurretPrototype;
class SpaceshipPrototype;

namespace resourceManager
{
    void playSound(const std::string& name);
    const sf::SoundBuffer &getSoundBuffer(const std::string &name);
    const sf::Font& getFont(const std::string& name);
    const json& getJSON(const std::string& name);
    const TurretPrototype& getTurretPrototype(const std::string& name);
    const SpaceshipPrototype &getSpaceshipPrototype(const std::string &name);
}
#endif /* !RESOURSEMANAGER_H_ */
