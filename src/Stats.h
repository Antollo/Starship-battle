#ifndef STATS_H_
#define STATS_H_

#include "Object.h"
#include "Spaceship.h"

class Stats
{
public:
    static void addTeam(const std::wstring &name)
    {
        nameToStats.try_emplace(name);
    }
    static bool joinTeam(const std::wstring &name, const Object::ObjectId &id)
    {
        if (id == 0)
            return false;
        idToName[id] = name;
        auto stats = nameToStats.find(name);
        if (stats == nameToStats.end())
            return false;
        auto object = Object::objects.find(id);
        if (object == Object::objects.end())
            return false;
        auto spaceship = dynamic_cast<Spaceship*>(object->second.get());
        if (spaceship == nullptr)
            return false;
        stats->second.pilots.push_back(spaceship->getPlayerId());
        std::sort(stats->second.pilots.begin(), stats->second.pilots.end());
        auto last = std::unique(stats->second.pilots.begin(), stats->second.pilots.end());
        stats->second.pilots.erase(last, stats->second.pilots.end());
        return true;
    }
    static void damageDealt(const Object::ObjectId &id, const float &damage)
    {
        auto team = idToName.find(id);
        if (team == idToName.end())
            return;
        auto stats = nameToStats.find(team->second);
        if (stats == nameToStats.end())
            return;
        stats->second.damage += damage;
    }
    static void hpLost(const Object::ObjectId &id, const float &hp)
    {
        auto team = idToName.find(id);
        if (team == idToName.end())
            return;
        auto stats = nameToStats.find(team->second);
        if (stats == nameToStats.end())
            return;
        stats->second.hp += hp;
    }

    template <class F>
    static void forEach(F &&f)
    {
        for (const auto &stats : nameToStats)
            f(stats.first, stats.second);
    }

    float damage = 0.f;
    float hp = 0.f;
    std::vector<std::wstring> pilots;

private:
    inline static std::map<Object::ObjectId, std::wstring> idToName;
    inline static std::map<std::wstring, Stats> nameToStats;
};

#endif /* !STATS_H_ */
