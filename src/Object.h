#ifndef OBJECT_H_
#define OBJECT_H_

#include <memory>
#include <deque>
#include <list>
#include <algorithm>
#include <iostream>
#include <random>
#include <cstdint>
#include <string>
#include <SFML/Graphics.hpp>
#include "Vec2f.h"
#include "resourceManager.h"

using namespace std::string_literals;

class Map;

class RenderSerializable;

class RenderSerializerBase
{
public:
    //virtual void draw(const RenderSerializable &drawable, const sf::RenderStates &states = sf::RenderStates::Default) noexcept = 0;
    virtual void draw(const RenderSerializable &drawable, const sf::RenderStates &states = sf::RenderStates::Default, const Vec2f &position = {0.f, 0.f}, const Vec2f &linearVelocity = {0.f, 0.f}, float angularVelocity = 0.f) noexcept = 0;
    //virtual void draw(const sf::VertexArray &vertexArray, const sf::RenderStates &states = sf::RenderStates::Default) noexcept = 0;
    virtual void draw(const sf::VertexArray &vertexArray, const sf::RenderStates &states, const Vec2f &position, const Vec2f &linearVelocity, float angularVelocity) noexcept = 0;
};

class RenderSerializable
{
private:
    friend class RenderSerializerBase;
    friend class RenderSerializer;
    virtual void draw(RenderSerializerBase &renderSerializer, sf::RenderStates states, const Vec2f &position, const Vec2f &linearVelocity, float angularVelocity) const noexcept = 0;
};

class Object : public sf::Drawable, public sf::Transformable, public RenderSerializable
{
public:
    using ObjectId = std::int32_t;
    using ObjectsContainer = std::map<ObjectId, std::unique_ptr<Object>>;
    enum class TypeId
    {
        Invalid,
        PrototypeSpaceship,
        Spaceship,
        Bullet,
        Bot,
        Rock,
        Shield
    };
    virtual const TypeId getTypeId() const { return TypeId::Invalid; };
    virtual Vec2f getCenterPosition() const = 0;
    virtual Vec2f getLinearVelocity() const = 0;
    virtual float getAngularVelocity() const = 0;
    Object() : destroy(false), id(++counter)
    {
        if (counter == std::numeric_limits<Object::ObjectId>::max())
            counter = 1;
    };
    virtual ObjectId getId() { return id; }
    virtual void process() = 0;
    void checkDestroy()
    {
        if (destroy)
            onDestroy();
    }
    bool destroy;
    static Object::ObjectId counter;
    static ObjectsContainer objects;
    static ObjectId thisPlayerId;
    static b2World world;
    static std::random_device rd;
    static std::mt19937 mt;
    static std::uniform_real_distribution<float> rng025, rng01;
    static constexpr float worldScale = 40.f;
    static constexpr float worldLimits = 12000.f;
    virtual ~Object() {}
    static void processAll()
    {
        for (const auto &object : Object::objects)
            object.second->process();

        ObjectsContainer::iterator i, j;
        for (auto i = Object::objects.begin(); i != Object::objects.end();)
        {
            j = i++;
            j->second->checkDestroy();
        }
    }
    static void destroyAll(bool firstPhase = true)
    {
        for (auto &object : Object::objects)
            object.second->destroy = true;
        processAll();
        if (firstPhase)
            destroyAll(false);
    }
    static void setMap(Map *m);
    static const Map& getMap()
    {
        return *map;
    }

protected:
    ObjectId id;
    sf::VertexArray polygon;

private:
    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states);
    }
    void draw(RenderSerializerBase &target, sf::RenderStates states, const Vec2f &position, const Vec2f &linearVelocity, float angularVelocity) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states, getCenterPosition(), getLinearVelocity(), getAngularVelocity());
    }

    void onDestroy()
    {
        //objects.erase(getId()); Bullets represents owners;
        objects.erase(id);
        /*objects.erase(std::find_if(objects.begin(), objects.end(), [this](const auto& it) {
            return it.second.get() == this;
        }));*/
    }

    static Map *map;
};

#endif
