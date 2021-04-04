#include "resourceManager.h"
#include <filesystem>

const sf::SoundBuffer &resourceManager::getSoundBuffer(const std::string &name)
{
    static std::map<std::string, sf::SoundBuffer> soundBufferMap;
    if (!soundBufferMap.count(name))
    {
        soundBufferMap[name] = sf::SoundBuffer();
        soundBufferMap[name].loadFromFile(name);
    }
    return soundBufferMap[name];
}

const json &resourceManager::getJSON(const std::string &name)
{
    static std::map<std::string, json> jsonMap;
    if (name == "hash")
    {
        for (auto &p : std::filesystem::directory_iterator("."))
            if (p.path().extension() == ".json")
                getJSON(p.path().stem().string());

        std::string str;
        for (const auto &el : jsonMap)
            str += el.second.dump();

        static json ret = {{"sha256", digestpp::sha256().absorb(str).hexdigest()}};
        return ret;
    }
    else if (!jsonMap.count(name))
    {
        std::ifstream file(name + ".json");
        if (!file.good())
            throw std::runtime_error("File " + name + ".json not found.");
        jsonMap[name] = json::parse(file);
    }
    return jsonMap[name];
}

void resourceManager::playSound(const std::string &name)
{
    static std::array<sf::Sound, 10> sounds;
    static std::size_t i = 0;
    sounds[i].setBuffer(getSoundBuffer(name));
    sounds[i++].play();
    i %= 10;
}

const sf::Font &resourceManager::getFont(const std::string &name)
{
    static std::map<std::string, sf::Font> fontMap;
    if (!fontMap.count(name))
    {
        fontMap[name] = sf::Font();
        fontMap[name].loadFromFile(name);
    }
    return fontMap[name];
}