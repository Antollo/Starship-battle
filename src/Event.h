#ifndef EVENT_H_
#define EVENT_H_

#include <cassert>
#include "Object.h"
#include "Vec2f.h"
#include "Shape.h"

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
        Ping,
        MissingShape
    };
    UpEvent(Type newType, Object::ObjectId newTargetId, bool newState)
        : type(newType), targetId(newTargetId), state(newState), coords(Vec2f(0.f, 0.f)), command(L"#")
    {
        assert(type != Type::AimCoords && type != Type::Command);
    }

    UpEvent(Type newType, Object::ObjectId newTargetId, const Vec2f &newCoords)
        : type(newType), targetId(newTargetId), state(false), coords(newCoords), command(L"#")
    {
        assert(type == Type::AimCoords);
    }

    UpEvent(Type newType, const std::wstring &newCommand)
        : type(newType), targetId(-1), state(false), coords(Vec2f(0.f, 0.f)), command(newCommand)
    {
        assert(type == Type::Command);
    }

    UpEvent(Type newType)
        : type(newType), targetId(-1), state(false), coords(Vec2f(0.f, 0.f)), command(L"#")
    {
        assert(type == Type::Ping);
    }

    UpEvent(Type newType, Object::ObjectId newTargetId)
        : type(newType), targetId(newTargetId), state(false), coords(Vec2f(0.f, 0.f)), command(L"#")
    {
        assert(type == Type::MissingShape);
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
        Pong,
        MissingShape
    };

    DownEvent(DownEvent &&downEvent)
    {
        type = downEvent.type;
        std::swap(polygons, downEvent.polygons);
        std::swap(players, downEvent.players);
        std::swap(vertices, downEvent.vertices);
        vec = downEvent.vec;
        explosion = downEvent.explosion;
        std::swap(message, downEvent.message);
    }

    DownEvent(const DownEvent &downEvent) = default;

    DownEvent &operator=(DownEvent &&downEvent)
    {
        type = downEvent.type;
        std::swap(polygons, downEvent.polygons);
        std::swap(players, downEvent.players);
        std::swap(vertices, downEvent.vertices);
        vec = downEvent.vec;
        explosion = downEvent.explosion;
        std::swap(message, downEvent.message);
        return *this;
    }

    DownEvent(const Type &newType)
        : type(newType), message(L"#") {}

    DownEvent(const Type &newType, ServerShape *newShape, Shape::IdType newShapeId)
        : type(newType), message(L"#"), shape(newShape), shapeId(newShapeId) {}

    DownEvent()
        : type(Type::Invalid), message(L"#") {}

    Type type;

    class Polygon : public sf::Drawable
    {
    public:
        Polygon() {}
        Polygon(Shape::IdType s, float r, Vec2f p, Vec2f lv, float av)
            : shape(s), rotation(r), position(p), linearVelocity(lv), angularVelocity(av) {}

        void calculateTransform()
        {
            /*
            TODO: assign elements in matrix directly
            float angle = rotation / 180.f * pi;
            float cosine = std::cos(angle);
            float sine = std::sin(angle);
            float tx = -origin.x * cosine - origin.y * sine + position.x;
            float ty = origin.x * sine - origin.y * cosine + position.y;

            transform = sf::Transform(cosine, sine, tx,
                                      -sine, cosine, ty,
                                      0.f, 0.f, 1.f);*/

            sf::Vector2f origin = ClientShape::getShape(shape).origin;
            transform = sf::Transform::Identity;
            transform.translate(position).rotate(rotation).translate(-origin);
        }

        void update(float delta)
        {
            position += linearVelocity * delta;
            rotation += angularVelocity * delta;
            calculateTransform();
        }

        Polygon(const Polygon &) = default;
        Polygon(Polygon &&) = default;
        Polygon &operator=(const Polygon &) = default;
        Polygon &operator=(Polygon &&) = default;

        sf::Transform transform;
        Vec2f position;
        Vec2f linearVelocity;
        float angularVelocity, rotation;
        Shape::IdType shape;

    private:
        // static inline const sf::Color semiTransparentBlack = sf::Color(0, 0, 0, 200);
        virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const
        {
            states.transform *= transform;
            ClientShape &ref = ClientShape::getShape(shape);
            target.draw(ref.body, states);
            target.draw(ref.outline, states);
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
        std::vector<Player>::iterator it = std::find_if(players.begin(), players.end(), [&id](const Player &player)
                                                        { return player.id == id; });
        if (it == players.end())
            return nullptr;
        return &(*it);
    }

    std::vector<Polygon> polygons;
    std::vector<Player> players;
    std::vector<sf::Vector2f> vertices;
    sf::Vector2f vec;
    ServerShape *shape;
    Shape::IdType shapeId;
    uint8_t explosion;
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
    // const float *matrix;
    // sf::Vector2f position;
    packet << static_cast<std::uint8_t>(ev.type);
    if (ev.type == DownEvent::Type::DirectDraw)
    {
        packet << static_cast<std::uint16_t>(ev.polygons.size());
        for (const DownEvent::Polygon &polygon : ev.polygons)
        {
            // packet << static_cast<std::uint8_t>(polygon.vertices.getPrimitiveType()) << static_cast<std::uint8_t>(polygon.vertices.getVertexCount() - 1);
            packet << polygon.shape << polygon.rotation << polygon.position << polygon.linearVelocity << polygon.angularVelocity;
            /*for (size_t i = 0; i < polygon.vertices.getVertexCount() - 1; i++)
            {
                //TODO
                position = polygon.vertices[i].position; //polygon.states.transform.transformPoint(polygon.vertices[i].position);
                packet << position;
            }*/
        }
        packet << static_cast<std::uint16_t>(ev.players.size());
        for (const DownEvent::Player &player : ev.players)
        {
            packet << player.id << player.playerId << player.position << player.linearVelocity << player.reload << player.aimState << player.hp << player.maxHp;
        }
    }
    else if (ev.type == DownEvent::Type::MissingShape)
    {
        std::uint16_t size;
        packet << ev.shapeId;
        packet << ev.shape->origin;
        packet << std::uint16_t(ev.shape->vertices.size());
        for (const auto &vertex : ev.shape->vertices)
            packet << vertex;
    }
    else
        packet << ev.vec << ev.explosion << ev.message;
    return packet;
}

inline sf::Packet &operator>>(sf::Packet &packet, DownEvent &ev)
{
    //float a00, a01, a02, a10, a11, a12, a20, a21, a22;
    std::uint8_t type;
    std::uint16_t size;
    packet >> type;
    ev.type = static_cast<DownEvent::Type>(type);
    if (ev.type == DownEvent::Type::DirectDraw)
    {
        packet >> size;
        ev.polygons.resize(size);
        for (DownEvent::Polygon &polygon : ev.polygons)
        {
            packet >> polygon.shape >> polygon.rotation >> polygon.position >> polygon.linearVelocity >> polygon.angularVelocity;
        }
        packet >> size;
        ev.players.resize(size);
        for (DownEvent::Player &player : ev.players)
        {
            packet >> player.id >> player.playerId >> player.position >> player.linearVelocity >> player.reload >> player.aimState >> player.hp >> player.maxHp;
        }
    }
    else if (ev.type == DownEvent::Type::MissingShape)
    {
        std::uint16_t size;
        packet >> ev.shapeId;
        packet >> ev.vec;
        packet >> size;
        ev.vertices.resize(size);
        for (auto &vertex : ev.vertices)
            packet >> vertex;
    }
    else
        packet >> ev.vec >> ev.explosion >> ev.message;
    return packet;
}

#endif
