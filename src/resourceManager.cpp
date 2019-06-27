#include "resourceManager.h"

sf::SoundBuffer& getSoundBuffer(const std::string& name)
{
    static std::map<std::string, sf::SoundBuffer> soundBufferMap;
    if (!soundBufferMap.count(name))
    {
        soundBufferMap[name] = sf::SoundBuffer();
        soundBufferMap[name].loadFromFile(name);
    }
    return soundBufferMap[name];
}

void resourceManager::playSound(const std::string& name)
{
    static std::array<sf::Sound, 10> sounds;
    static std::size_t i = 0;
    sounds[i].setBuffer(getSoundBuffer(name));
    sounds[i++].play();
    i %= 10;
}

const sf::Font& resourceManager::getFont(const std::string& name)
{
    static std::map<std::string, sf::Font> fontMap;
    if (!fontMap.count(name))
    {
        fontMap[name] = sf::Font();
        fontMap[name].loadFromFile(name);
    }
    return fontMap[name];
}