#ifndef EVENT_H_
#define EVENT_H_

#include <cassert>
#include "Object.h"
#include "Vec2f.h"

class UpEvent {
public:
    enum class Type {
        Invalid, Forward, Left, Right, Aim, Shoot, AimCoords, Command
    };
    UpEvent(const Type& newType, const Object::ObjectId& newTargetId, const bool& newState)
        : type(newType), targetId(newTargetId), state(newState), coords(Vec2f(0.f, 0.f)), command(L"#")
    {
        assert(type != Type::AimCoords && type != Type::Command);
    }

    UpEvent(const Type& newType, const Object::ObjectId& newTargetId, const Vec2f& newCoords)
        : type(newType), targetId(newTargetId), state(false), coords(newCoords), command(L"#") 
    {
        assert(type == Type::AimCoords);
    }

    UpEvent(const Type& newType, const std::wstring& newCommand)
        : type(newType), targetId(-1), state(false), coords(Vec2f(0.f, 0.f)), command(newCommand)
    {
        assert(type == Type::Command);
    }

    UpEvent()
        : type(Type::Invalid), targetId(-1), state(false), coords(Vec2f(0.f, 0.f)), command(L"#") {}
    
    Type type;
    Object::ObjectId targetId;
    bool state;
    Vec2f coords;
    std::wstring command;
};

inline sf::Packet& operator <<(sf::Packet& packet, const UpEvent& ev)
{
    return packet << (std::int32_t)ev.type << ev.targetId << ev.state << ev.coords << ev.command;
}

inline sf::Packet& operator >>(sf::Packet& packet, UpEvent& ev)
{
    std::int32_t type;
    packet >> type >> ev.targetId >> ev.state >> ev.coords >> ev.command;
    ev.type = (UpEvent::Type)type;
    return packet;
}

class DownEvent {
public:
    enum class Type {
        Invalid, DirectDraw, Collision, Message, Response
    };

    DownEvent(const Type& newType)
        : type(newType), message(L"#") {}

    DownEvent()
        : type(Type::Invalid), message(L"#") {}

    Type type;

    class Polygon {
    public:
        sf::VertexArray vertices;
        sf::RenderStates states;
    };

    class Player {
    public:
        Object::ObjectId id;
        std::wstring playerId; 
        Vec2f coords;
        float reload;
        std::int32_t hp;
        std::int32_t maxHp;
    };

    const Player* findPlayer(Object::ObjectId id)
    {
        std::vector<Player>::iterator it = std::find_if(players.begin(), players.end(), [&id](const Player& player) {
            return player.id == id;
        });
        if (it == players.end())
            return nullptr;
        return &(*it);
    }

    std::vector<Polygon> polygons;
    std::vector<Player> players;
    Vec2f collision;
    bool explosion;
    std::wstring message;
};

inline sf::Packet& operator <<(sf::Packet& packet, const DownEvent& ev)
{
    const float* matrix;
    packet << (std::int32_t)ev.type << (std::int32_t)ev.polygons.size();
    for(const DownEvent::Polygon& polygon : ev.polygons)
    {
        matrix = polygon.states.transform.getMatrix();
        packet << matrix[0] << matrix[4] << matrix[12]
               << matrix[1] << matrix[5] << matrix[13]
               << matrix[3] << matrix[7] << matrix[15];
        packet << (std::int32_t)polygon.vertices.getPrimitiveType() << (std::int32_t)polygon.vertices.getVertexCount();
        for (size_t i = 0; i < polygon.vertices.getVertexCount(); i++)
        {
            packet << polygon.vertices[i].position.x << polygon.vertices[i].position.y
            << polygon.vertices[i].color.r << polygon.vertices[i].color.g
            << polygon.vertices[i].color.b << polygon.vertices[i].color.a; 
        }
    }
    packet << (std::int32_t)ev.players.size();
    for(const DownEvent::Player& player : ev.players)
    {
        packet << player.id << player.playerId << player.coords << player.reload << player.hp << player.maxHp;
    }
    packet << ev.collision.x << ev.collision.y << ev.explosion << ev.message;
    return packet;
}

inline sf::Packet& operator >>(sf::Packet& packet, DownEvent& ev)
{
    float a00, a01, a02, a10, a11, a12, a20, a21, a22;
    std::int32_t type, size;
    packet >> type >> size;
    ev.type = (DownEvent::Type)type;
    ev.polygons.resize(size);
    for(DownEvent::Polygon& polygon : ev.polygons)
    {
        packet >> a00 >> a01 >> a02 >> a10 >> a11 >> a12 >> a20 >> a21 >> a22;
        polygon.states.transform = sf::Transform(a00, a01, a02, a10, a11, a12, a20, a21, a22);
        packet >> type >> size;
        polygon.vertices.setPrimitiveType((sf::PrimitiveType)type);
        polygon.vertices.resize(size);
        for (std::int32_t i = 0; i < size; i++)
        {
            packet >> polygon.vertices[i].position.x >> polygon.vertices[i].position.y
            >> polygon.vertices[i].color.r >> polygon.vertices[i].color.g
            >> polygon.vertices[i].color.b >> polygon.vertices[i].color.a; 
        }
    }
    packet >> size;
    ev.players.resize(size);
    for(DownEvent::Player& player : ev.players)
    {
        packet >> player.id >> player.playerId >> player.coords >> player.reload >> player.hp >> player.maxHp;
    }
    packet >> ev.collision.x >> ev.collision.y >> ev.explosion >> ev.message;
    return packet;
}


#endif
