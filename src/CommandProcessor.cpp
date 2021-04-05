#include "CommandProcessor.h"
#include "Map.h"

std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> CommandProcessor::converter;

CommandProcessor::CommandProcessor()
{
    bind(L"create", [](std::wstring shipType, std::wstring pilotName) {
        Object::ObjectId id = Spaceship::create(CommandProcessor::converter.to_bytes(shipType), pilotName)->getId();
        return L"setThisPlayerId "s + std::to_wstring(id) + L" print Spaceship is ready.\n"s;
    });

    bind(L"create-bot", [](std::wstring shipType) {
        Bot::create(CommandProcessor::converter.to_bytes(shipType));
        return L"print Bot is ready.\n"s;
    });

    bind(L"create-random-bot", [](std::wstring shipType) {
        json jsonObject = resourceManager::getJSON("config");
        std::vector<std::string> spaceships = jsonObject["spaceships"].get<std::vector<std::string>>();
        std::size_t i = float(spaceships.size()) * Object::uniformRNG<0, 1, 1, 1>();
        Bot::create(spaceships[i]);
        return L"print Bot is ready.\n"s;
    });

    bind(L"create-bots", []() {
        json jsonObject = resourceManager::getJSON("config");
        std::vector<std::string> spaceships = jsonObject["spaceships"].get<std::vector<std::string>>();
        std::size_t i = 0;
        for (const auto &name : spaceships)
        {
            Bot::create(name);
        }
        return L"print Bots are ready.\n"s;
    });

    bind(L"create-team", [](std::wstring teamName) {
        Stats::addTeam(teamName);
        return L"print Team " + teamName + L" created.\n"s;
    });

    bind(L"join-team", [](std::wstring teamName, Object::ObjectId id) {
        if (Stats::joinTeam(teamName, id))
            return L"print Welcome in "s + teamName + L".\n";
        return L"print Joining team failed.\nYou don't have a spaceship or there's no such team.\n"s;
    });

    bind(L"team-stats", []() {
        std::wstring res = L"print Stats:\n";
        Stats::forEach([&res](const std::wstring &teamName, const Stats &stats) {
            res += teamName + L"\n";
            res += L"    Damage dealt: " + std::to_wstring(std::lround(stats.damage)) + L"\n";
            res += L"    Hp lost:      " + std::to_wstring(std::lround(stats.hp)) + L"\n";
            res += L"    Pilots:       ";
            for (const auto &pilotName : stats.pilots)
                res += pilotName + L" ";
            res += L"\n";
        });
        return res;
    });

    bind(L"wish", [](Object::ObjectId id) -> std::wstring {
        static constexpr auto message = L"print Making wish failed.\nYou don't have a spaceship.\n";
        if (id == 0)
            return message;
        auto object = Object::objects.find(id);
        if (object == Object::objects.end())
            return message;
        auto spaceship = dynamic_cast<Spaceship *>(object->second.get());
        if (spaceship == nullptr)
            return message;

        int r = Object::uniformRNG<0, 1, 10, 1>();

        switch (r)
        {
        case 0:
        {
            auto pilotName = spaceship->getPlayerId();
            Object::objects.erase(id);
            Spaceship::create("V7-C5", pilotName, id);
            return LR"#(print 
   ________________________
 / \                       \.
|   |         *****        |.
 \_ |  V7 Constellation 5  |.
    |       (V7-C5)        |.
    |   5 star battleship  |.
    |   ___________________|___
    |  /                      /.
    \_/______________________/.
)#"s;
        }
        case 1:
        case 2:
        {
            spaceship->increaseArmor(1.f);
            spaceship->inceaseHp(-160.f);
            return LR"#(print 
   _______________________
 / \                      \.
|   |        ****         |.
 \_ |  Divine Protection  |.
    |  +1 armor, -160 hp  |.
    |    4 star upgrade   |.
    |   __________________|___
    |  /                     /.
    \_/_____________________/.
)#"s;
        }
        case 3:
        case 4:
        {
            spaceship->inceaseHp(320.f);
            return LR"#(print 
   __________________
 / \                 \.
|   |      ***       |.
 \_ |      Heal      |.
    |    +320 hp     |.
    |  3 star spell  |.
    |   _____________|___
    |  /               /.
    \_/_______________/.
)#"s;
        }

        default:
            spaceship->inceaseHp(-160.f);
            return LR"#(print 
   _______________
 / \              \.
|   |      *       |.
 \_ |  Nothing XD  |.
    |   -160 hp    |.
    |    1 star    |.
    |   ___________|__
    |  /             /.
    \_/_____________/.
)#"s;
        }
    });

    bind(L"borders", []() {
        Borders::create();
        return L""s;
    });

    bind(L"bots-battle", [this]() {
        Object::destroyAll();
        Object::setMap(Map::create());
        /*std::size_t n = 60;
            while (n--)
                Rock::create();*/

        call(L"borders"s);

        int n = 10;
        while (n--)
            call(L"create-bots"s);

        for (const auto &obj : Object::objects)
            if (obj.second->getTypeId() == Object::TypeId::Bot)
                Bot::allTarget(obj.second->getId());

        return L"print Bots battle started.\n"s;
    });

    bind(L"credits", []() {
        return L"print "
               L"SFML          - www.sfml-dev.org\n"
               L"Box2D         - box2d.org\n"
               L"Box2D-MT      - github.com/jhoffman0x/Box2D-MT\n"
               L"nlohmann/json - github.com/nlohmann/json\n"
               L"Electron      - electronjs.org\n"
               L"Ubuntu Mono   - fonts.google.com/specimen/Ubuntu+Mono\n"
               L"freesound     - freesound.org/people/spaciecat/sounds/456779\n"
               L"                freesound.org/people/Diboz/sounds/213925\n"
               L"                freesound.org/people/ryansnook/sounds/110111\n"
               L"                freesound.org/people/debsound/sounds/437602\n"s;
    });

    bind(L"delete", []() {
        return L"print Monika.chr deleted successfully.\n"s;
    });

    bind(L"list-spaceships", []() {
        std::wstring res = L"print "s;
        const json &jsonObject = resourceManager::getJSON("config");
        std::vector<std::string> spaceships = jsonObject["spaceships"].get<std::vector<std::string>>();
        for (const auto &name : spaceships)
        {
            res += CommandProcessor::converter.from_bytes(name.c_str()) + L"\n"s;
        }
        return res;
    });

    bind(L"help", []() {
        return L"print \n"
               L"Spaceship commander command prompt\n"
               L"Remote server, version from " +
               CommandProcessor::converter.from_bytes(__DATE__) + L" " + CommandProcessor::converter.from_bytes(__TIME__) +
               L"\n\n"
               L"Use W key to accelerate\n"
               L"Use A and D keys to rotate\n"
               L"Use mouse to aim\n"
               L"Use left mouse button to shoot\n"
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
               L"    message [body]                       (aliased as 'm')\n"
               L"    delete                               \n"
               L"    credits                              \n"
               L"    grid [on/off]                        (aliased as 'g')\n"
               L"    wish                                 \n";
    });

    alias(L"create", L"c");
    alias(L"create-bot", L"cb");
    alias(L"create-bots", L"cbs");
    alias(L"list-spaceships", L"ls");
    alias(L"help", L"h");
    alias(L"create-team", L"ct");
    alias(L"join-team", L"jt");
    alias(L"team-stats", L"s");
}