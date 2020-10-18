#ifndef EVENT_H_
#define EVENT_H_

#include <cassert>
#include "Object.h"
#include "Vec2f.h"

class UpEvent
{
public:
    enum class Type
    {
        Invalid,
        Forward,
        Left,
        Right,
        Shoot,
        AimCoords,
        Command,
        Ping
    };
    UpEvent(const Type &newType, const Object::ObjectId &newTargetId, const bool &newState)
        : type(newType), targetId(newTargetId), state(newState), coords(Vec2f(0.f, 0.f)), command(L"#")
    {
        assert(type != Type::AimCoords && type != Type::Command);
    }

    UpEvent(const Type &newType, const Object::ObjectId &newTargetId, const Vec2f &newCoords)
        : type(newType), targetId(newTargetId), state(false), coords(newCoords), command(L"#")
    {
        assert(type == Type::AimCoords);
    }

    UpEvent(const Type &newType, const std::wstring &newCommand)
        : type(newType), targetId(-1), state(false), coords(Vec2f(0.f, 0.f)), command(newCommand)
    {
        assert(type == Type::Command);
    }

    UpEvent(const Type &newType)
        : type(newType), targetId(-1), state(false), coords(Vec2f(0.f, 0.f)), command(L"#")
    {
        assert(type == Type::Ping);
    }

    UpEvent()
        : type(Type::Invalid), targetId(-1), state(false), coords(Vec2f(0.f, 0.f)), command(L"#") {}

    Type type;
    Object::ObjectId targetId;
    bool state;
    Vec2f coords;
    std::wstring command;
};

inline sf::Packet &operator<<(sf::Packet &packet, const UpEvent &ev)
{
    return packet << static_cast<std::uint8_t>(ev.type) << ev.targetId << ev.state << ev.coords << ev.command;
}

inline sf::Packet &operator>>(sf::Packet &packet, UpEvent &ev)
{
    std::uint8_t type;
    packet >> type >> ev.targetId >> ev.state >> ev.coords >> ev.command;
    ev.type = static_cast<UpEvent::Type>(type);
    return packet;
}

class DownEvent
{
public:
    enum class Type
    {
        Invalid,
        DirectDraw,
        Collision,
        Message,
        Response,
        Pong
    };

    DownEvent(DownEvent &&downEvent)
    {
        type = downEvent.type;
        std::swap(polygons, downEvent.polygons);
        std::swap(players, downEvent.players);
        collision = downEvent.collision;
        explosion = downEvent.explosion;
        std::swap(message, downEvent.message);
    }

    DownEvent(const DownEvent &downEvent) = default;

    DownEvent &operator=(DownEvent &&downEvent)
    {
        type = downEvent.type;
        std::swap(polygons, downEvent.polygons);
        std::swap(players, downEvent.players);
        collision = downEvent.collision;
        explosion = downEvent.explosion;
        std::swap(message, downEvent.message);
        return *this;
    }

    DownEvent(const Type &newType)
        : type(newType), message(L"#") {}

    DownEvent()
        : type(Type::Invalid), message(L"#") {}

    Type type;

    class Polygon : public sf::Drawable
    {
    public:
        Polygon() {}
        Polygon(sf::VertexArray v, sf::RenderStates s, Vec2f p, Vec2f lv, float av)
            : vertices(v), states(s), position(p), linearVelocity(lv), angularVelocity(av) {}

        Polygon(const Polygon &) = default;
        Polygon(Polygon &&) = default;
        Polygon &operator=(const Polygon &) = default;
        Polygon &operator=(Polygon &&) = default;

        mutable sf::VertexArray vertices;
        sf::RenderStates states;
        Vec2f position;
        Vec2f linearVelocity;
        float angularVelocity;

    private:
        static inline const sf::Color semiTransparentBlack = sf::Color(0, 0, 0, 160);
        virtual void draw(sf::RenderTarget &target, sf::RenderStates _) const
        {
            if (vertices.getPrimitiveType() == sf::LineStrip)
            {
                vertices.setPrimitiveType(sf::TriangleFan);
                for (size_t i = 0; i < vertices.getVertexCount(); i++)
                    vertices[i].color = semiTransparentBlack;
                target.draw(vertices, states);
                vertices.setPrimitiveType(sf::LineStrip);
                for (size_t i = 0; i < vertices.getVertexCount(); i++)
                    vertices[i].color = sf::Color::White;
                target.draw(vertices, states);
            }
            else
                target.draw(vertices, states);
        }
    };

    class Player
    {
    public:
        Object::ObjectId id;
        std::wstring playerId;
        Vec2f position;
        Vec2f linearVelocity;
        float reload;
        bool aimState;
        std::int16_t hp;
        std::int16_t maxHp;
    };

    const Player *findPlayer(Object::ObjectId id)
    {
        std::vector<Player>::iterator it = std::find_if(players.begin(), players.end(), [&id](const Player &player) {
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

inline sf::Packet &operator<<(sf::Packet &packet, const sf::Vector2f &v)
{
    packet << v.x << v.y;
    return packet;
}

inline sf::Packet &operator>>(sf::Packet &packet, sf::Vector2f &v)
{
    packet >> v.x >> v.y;
    return packet;
}

inline sf::Packet &operator<<(sf::Packet &packet, const DownEvent &ev)
{
    //const float *matrix;
    sf::Vector2f position;
    packet << static_cast<std::uint8_t>(ev.type) << static_cast<std::uint16_t>(ev.polygons.size());
    for (const DownEvent::Polygon &polygon : ev.polygons)
    {
        /*matrix = polygon.states.transform.getMatrix();
        packet << matrix[0] << matrix[4] << matrix[12]
               << matrix[1] << matrix[5] << matrix[13]
               << matrix[3] << matrix[7] << matrix[15];*/
        packet << static_cast<std::uint8_t>(polygon.vertices.getPrimitiveType()) << static_cast<std::uint8_t>(polygon.vertices.getVertexCount());
        packet << polygon.position << polygon.linearVelocity << polygon.angularVelocity;
        for (size_t i = 0; i < polygon.vertices.getVertexCount(); i++)
        {
            position = polygon.states.transform.transformPoint(polygon.vertices[i].position);
            packet << position;
            /*packet << polygon.vertices[i].position.x << polygon.vertices[i].position.y;
            << polygon.vertices[i].color.r << polygon.vertices[i].color.g
            << polygon.vertices[i].color.b << polygon.vertices[i].color.a;*/
        }
    }
    packet << static_cast<std::uint16_t>(ev.players.size());
    for (const DownEvent::Player &player : ev.players)
    {
        packet << player.id << player.playerId << player.position << player.linearVelocity << player.reload << player.aimState << player.hp << player.maxHp;
    }
    packet << ev.collision.x << ev.collision.y << ev.explosion << ev.message;
    return packet;
}

inline sf::Packet &operator>>(sf::Packet &packet, DownEvent &ev)
{
    //float a00, a01, a02, a10, a11, a12, a20, a21, a22;
    std::uint8_t type;
    std::uint16_t size;
    packet >> type >> size;
    ev.type = static_cast<DownEvent::Type>(type);
    ev.polygons.resize(size);
    for (DownEvent::Polygon &polygon : ev.polygons)
    {
        /*packet >> a00 >> a01 >> a02 >> a10 >> a11 >> a12 >> a20 >> a21 >> a22;
        polygon.states.transform = sf::Transform(a00, a01, a02, a10, a11, a12, a20, a21, a22);*/
        polygon.states = sf::RenderStates::Default;
        packet >> type;
        polygon.vertices.setPrimitiveType(static_cast<sf::PrimitiveType>(type));
        packet >> type; // Actually size, but uint8_t
        packet >> polygon.position >> polygon.linearVelocity >> polygon.angularVelocity;
        polygon.vertices.resize(type);
        for (std::uint16_t i = 0; i < type; i++)
        {
            packet >> polygon.vertices[i].position;
            polygon.vertices[i].color = sf::Color::White; // There's only one color in this game
            /*>> polygon.vertices[i].color.r >> polygon.vertices[i].color.g
            >> polygon.vertices[i].color.b >> polygon.vertices[i].color.a;*/
        }
    }
    packet >> size;
    ev.players.resize(size);
    for (DownEvent::Player &player : ev.players)
    {
        packet >> player.id >> player.playerId >> player.position >> player.linearVelocity >> player.reload >> player.aimState >> player.hp >> player.maxHp;
    }
    packet >> ev.collision.x >> ev.collision.y >> ev.explosion >> ev.message;
    return packet;
}

#endif
