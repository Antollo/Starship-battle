#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#ifndef COMMANDPROCESSOR_H_
#define COMMANDPROCESSOR_H_

#include <map>
#include <functional>
#include <sstream>
#include <locale>
#include <codecvt>
#include <memory>
#include <type_traits>
#include "Console.h"
#include "Stats.h"

#include <string>
#include "Bot.h"
#include "resourceManager.h"

//using ArgsText = std::wstring;
//using ArgsStream = std::basic_stringstream<ArgsText::value_type>;

inline std::wstringstream &operator>>(std::wstringstream &lhs, std::wstringstream &rhs)
{
    rhs << lhs.rdbuf();
    return lhs;
}
class CallableBase
{
public:
    virtual std::wstring operator()(std::wstringstream &argsText) const = 0;
};

template <class... Args>
class Callable : public CallableBase
{
public:
    Callable(std::function<std::wstring(Args...)> &&newCallable) : callable(newCallable) {}
    std::wstring operator()(std::wstringstream &argsStream) const override
    {
        return call(argsStream, std::index_sequence_for<Args...>());
    }
    template <std::size_t... Is>
    std::wstring call(std::wstringstream &argsStream, std::index_sequence<Is...>) const
    {
        std::tuple<std::remove_reference_t<Args>...> t;
        // Trick from: https://stackoverflow.com/questions/6245735/pretty-print-stdtuple
        using swallow = int[];
        (void)swallow{0, (void(argsStream >> std::get<Is>(t)), 0)...};
        // End of trick
        return callable((std::get<Is>(t))...);
    }

private:
    std::function<std::wstring(Args...)> callable;
};

class CommandProcessor
{
public:
    template <class T>
    void bind(const std::wstring &key, T &&callback)
    {
        map.emplace(key, new Callable(std::function(callback)));
    }
    template <class T>
    void job(T &&callback)
    {
        jobs.emplace_back(new Callable(std::function(callback)));
    }
    void alias(const std::wstring &key, const std::wstring &alias)
    {
        map.emplace(alias, new Callable(std::function([this, key](std::wstringstream &args) {
                        return call(key, args);
                    })));
    }
    std::wstring call(const std::wstring &key, std::wstringstream &args)
    {
        if (map.count(key))
            return (*map[key])(args);
        return L"print " + key + L"-error: Command not found.\n" + L"Use 'help' to get help.\n";
    }
    std::wstring call(const std::wstring &str)
    {
        std::wstring key;
        inputsStream.clear();
        inputsStream.str(str);
        inputsStream >> key;
        try
        {
            return call(key, inputsStream);
        }
        catch (const std::exception &e)
        {
            return L"print " + key + L"-error: " + converter.from_bytes(e.what()) + L"\n" + L"Use 'help-" + key + L"' to get help.\n";
        }
    }
    void processJobs()
    {
        inputsStream.clear();
        for (it = jobs.begin(); it != jobs.end();)
        {
            jt = it++;
            try
            {
                if ((**jt)(inputsStream).empty())
                    jobs.erase(jt);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Job error: " << e.what() << std::endl;
            }
        }
    }
    static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    static inline void init(CommandProcessor& commandProcessor);

private:
    std::map<std::wstring, std::unique_ptr<CallableBase>> map;
    std::list<std::unique_ptr<CallableBase>> jobs;
    std::list<std::unique_ptr<CallableBase>>::iterator it, jt;
    std::wstringstream inputsStream;

    static constexpr float width = 20.f; // Half of width
    static constexpr float pos = 400.f;
};

inline void CommandProcessor::init(CommandProcessor& commandProcessor)
{
    commandProcessor.bind(L"create", [](std::wstring shipType, std::wstring pilotName) {
        Object::ObjectId id = Spaceship::create(CommandProcessor::converter.to_bytes(shipType), pilotName)->getId();
        return L"setThisPlayerId "s + std::to_wstring(id) + L" print Spaceship is ready.\n"s;
    });

    commandProcessor.bind(L"create-bot", [](std::wstring shipType) {
        Bot::create(CommandProcessor::converter.to_bytes(shipType));
        return L"print Bot is ready.\n"s;
    });

    commandProcessor.bind(L"create-random-bot", [](std::wstring shipType) {
        json jsonObject = resourceManager::getJSON("config");
        std::vector<std::string> spaceships = jsonObject["spaceships"].get<std::vector<std::string>>();
        std::size_t i = float(spaceships.size()) * Object::rng01(Object::mt);
        Bot::create(spaceships[i]);
        return L"print Bot is ready.\n"s;
    });

    commandProcessor.bind(L"create-bots", []() {
        json jsonObject = resourceManager::getJSON("config");
        std::vector<std::string> spaceships = jsonObject["spaceships"].get<std::vector<std::string>>();
        std::size_t i = 0;
        for (const auto &name : spaceships)
        {
            Bot::create(name);
        }
        return L"print Bots are ready.\n"s;
    });

    commandProcessor.bind(L"create-team", [](std::wstring teamName) {
        Stats::addTeam(teamName);
        return L"print Team " + teamName + L" created.\n"s;
    });

    commandProcessor.bind(L"join-team", [](std::wstring teamName, Object::ObjectId id) {
        if (Stats::joinTeam(teamName, id))
            return L"print Welcome in "s + teamName + L".\n";
        return L"print Joining team failed.\nYou don't have a spaceship or there's no such team.\n"s;
    });

    commandProcessor.bind(L"team-stats", [](std::wstring teamName, Object::ObjectId id) {
        std::wstring res = L"print Stats:\n";
        Stats::forEach([&res](const std::wstring& teamName, const Stats& stats){
            res += teamName + L"\n";
            res += L"    Damage dealt: " + std::to_wstring(std::lround(stats.damage)) + L"\n";
            res += L"    Hp lost:      " + std::to_wstring(std::lround(stats.hp)) + L"\n";
            res += L"    Pilots:       ";
            for (const auto& pilotName : stats.pilots)
                 res += pilotName + L" ";
            res += L"\n";
        });
        return res;
    });

    commandProcessor.bind(L"borders", []() {
        Rock::create({{-width, pos - width}, {width, pos + width}, {-width, -pos + width}, {width, -pos - width}}, pos, 0.f);
        Rock::create({{-width, pos + width}, {width, pos - width}, {-width, -pos - width}, {width, -pos + width}}, -pos, 0.f);
        Rock::create({{pos - width, -width}, {pos + width, width}, {-pos + width, -width}, {-pos - width, width}}, 0.f, pos);
        Rock::create({{pos + width, -width}, {pos - width, width}, {-pos - width, -width}, {-pos + width, width}}, 0.f, -pos);
        return L""s;
    });

    commandProcessor.bind(L"bots-battle", [&commandProcessor]() {
        Object::destroyAll();
        std::size_t n = 60;
        while (n--)
            Rock::create();
            
        commandProcessor.call(L"borders"s);

        n = 10;
        while(n--)
            commandProcessor.call(L"create-bots"s);

        for (const auto& obj : Object::objects)
            if (obj.second->getTypeId() == Object::TypeId::Bot)
                Bot::allTarget(obj.second->getId());
        
        return L"print Bots battle started.\n"s;
    });

    commandProcessor.bind(L"credits", []() {
        return L"print "
               L"SFML          - www.sfml-dev.org\n"
               L"Box2D         - box2d.org\n"
               L"nlohmann/json - github.com/nlohmann/json\n"
               L"Electron      - electronjs.org\n"
               L"Ubuntu Mono   - fonts.google.com/specimen/Ubuntu+Mono\n"
               L"freesound     - freesound.org/people/spaciecat/sounds/456779\n"
               L"                freesound.org/people/Diboz/sounds/213925\n"
               L"                freesound.org/people/ryansnook/sounds/110111\n"
               L"                freesound.org/people/debsound/sounds/437602\n"s;
    });

    commandProcessor.bind(L"delete", []() {
        return L"print Monika.chr deleted successfully.\n"s;
    });

    commandProcessor.bind(L"list-spaceships", []() {
        std::wstring res = L"print "s;
        const json& jsonObject = resourceManager::getJSON("config");
        std::vector<std::string> spaceships = jsonObject["spaceships"].get<std::vector<std::string>>();
        for (const auto &name : spaceships)
        {
            res += CommandProcessor::converter.from_bytes(name.c_str()) + L"\n"s;
        }
        return res;
    });

    commandProcessor.bind(L"beep", []() {
        resourceManager::playSound("glitch.wav");
        return L"print Server beeped.\n"s;
    });
    
    commandProcessor.bind(L"help", []() {
        return L"print \n"
               L"Spaceship commander command prompt\n"
               L"Remote server, version from " +
               CommandProcessor::converter.from_bytes(__DATE__) + L" " + CommandProcessor::converter.from_bytes(__TIME__) +
               L"\n\n"
               L"Use W key to accelerate\n"
               L"Use A and D keys to rotate\n"
               L"Use mouse right button to aim\n"
               L"Use mouse left button to shoot\n"
               L"Use mouse wheel to scale the view\n"
               L"Use tilde key to switch console input\n"
               L"Use up, down, and tab keys as in normal console\n\n"
               L"List of commands:\n"
               L"    create [spaceship type] [pilot name] (aliased as 'c')\n"
               L"    create-bot [spaceship type]          (aliased as 'cb')\n"
               L"    create-bots                          (aliased as 'cbs')\n"
               L"    create-random-bot                    \n"
               L"    list-spaceships                      (aliased as 'ls')\n"
               L"    help                                 (aliased as 'h')\n"
               L"    bots-battle                          \n"
               L"    create-team [team name]              (aliased as 'ct')\n"
               L"    join-team [team name]                (aliased as 'jt')\n"
               L"    team-stats                           (aliased as 's')\n"
               L"    delete                               \n"
               L"    credits                              \n"
               L"    beep                                 \n"s;
    });

    commandProcessor.alias(L"create", L"c");
    commandProcessor.alias(L"create-bot", L"cb");
    commandProcessor.alias(L"create-bots", L"cbs");
    commandProcessor.alias(L"list-spaceships", L"ls");
    commandProcessor.alias(L"help", L"h");
    commandProcessor.alias(L"create-team", L"ct");
    commandProcessor.alias(L"join-team", L"jt");
    commandProcessor.alias(L"team-stats", L"s");
}

#endif
