#ifndef OBJECT_H_
#define OBJECT_H_

#include <memory>
#include <deque>
#include <list>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <string>
#include <SFML/Graphics.hpp>
#include "Vec2f.h"
#include "resourceManager.h"
#include "Shape.h"
#include "Rng.h"
#include "world.h"

using namespace std::string_literals;

class Map;

class RenderSerializable;

class RenderSerializerBase
{
public:
    virtual void draw(const RenderSerializable &drawable) = 0;
    virtual void draw(Shape::IdType shape, sf::Angle rotation, const Vec2f &position, const Vec2f &linearVelocity, sf::Angle angularVelocity) = 0;
};

class RenderSerializable
{
private:
    friend class RenderSerializerBase;
    friend class RenderSerializer;
    virtual void draw(RenderSerializerBase &renderSerializer) const = 0;
};

class Object : public sf::Transformable, public RenderSerializable
{
public:
    using ObjectId = std::int32_t;
    using ObjectsContainer = std::map<ObjectId, std::unique_ptr<Object>>;
    enum class TypeId
    {
        Invalid,
        Spaceship,
        Bullet,
        Bot,
        Rock,
        Shield
    };
    virtual const TypeId getTypeId() const { return TypeId::Invalid; };
    virtual Vec2f getCenterPosition() const = 0;
    virtual Vec2f getLinearVelocity() const = 0;
    virtual sf::Angle getAngularVelocity() const = 0;
    Object() : destroy(false), id(++counter)
    {
        if (counter == std::numeric_limits<Object::ObjectId>::max())
            counter = 1;
    }
    Object(ObjectId objectId) : destroy(false), id(objectId != 0 ? objectId : ++counter)
    {
        if (counter == std::numeric_limits<Object::ObjectId>::max())
            counter = 1;
    }
    virtual ~Object() {}
    virtual ObjectId getId() { return id; }
    virtual void process(float delta) = 0;
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

    static void processAll(float delta)
    {
        for (const auto &object : Object::objects)
            object.second->process(delta);

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
        processAll(0);
        if (firstPhase)
            destroyAll(false);
    }

protected:
    ObjectId id;

private:
    void onDestroy()
    {
        objects.erase(id);
    }
};

#endif
