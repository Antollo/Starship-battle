#include <cmath>
#include <nlohmann/json.hpp>
#include <SFML/Network.hpp>
#include <Box2D/Box2D.h>
using json = nlohmann::json;

constexpr float pi = 3.14159265358979323846f;

class Vec2f
{
public:
    float x, y;
    Vec2f() : x(0.f), y(0.f) {}
    Vec2f(float X, float Y) : x(X), y(Y) {}
    Vec2f(const sf::Vector2f& vec) : x(vec.x), y(vec.y) {}
    Vec2f(const b2Vec2& vec) : x(vec.x), y(vec.y) {}
    static const Vec2f& asVec2f (const sf::Vector2f& vec)
    {
        return *reinterpret_cast<const Vec2f*>(&vec);
    }
    static const Vec2f& asVec2f (const b2Vec2& vec)
    {
        return *reinterpret_cast<const Vec2f*>(&vec);
    }
    operator sf::Vector2f&()
    {
        return *reinterpret_cast<sf::Vector2f*>(this);
    }
    operator b2Vec2&()
    {
        return *reinterpret_cast<b2Vec2*>(this);
    }
    operator const sf::Vector2f&() const
    {
        return *reinterpret_cast<const sf::Vector2f*>(this);
    }
    operator const b2Vec2&() const
    {
        return *reinterpret_cast<const b2Vec2*>(this);
    }
    float getSquaredDistance(const Vec2f& vec) const
    {
        return (x-vec.x)*(x-vec.x) + (y-vec.y)*(y-vec.y);
    }
    float getSquaredLength() const
    {
        return x*x + y*y;
    }
    float getLength() const
    {
        return std::sqrtf(x*x + y*y);
    }
};

inline void to_json(json& j, const Vec2f& vec) {
    j = json{
        {"x", vec.x}, 
        {"y", vec.y}
    };
}

inline void from_json(const json& j, Vec2f& vec) {
    vec.x = j.at("x").get<float>();
    vec.y = j.at("y").get<float>();
}

inline sf::Packet& operator <<(sf::Packet& packet, const Vec2f& vec)
{
    return packet << vec.x << vec.y;
}

inline sf::Packet& operator >>(sf::Packet& packet, Vec2f& vec)
{
    return packet >> vec.x >> vec.y;
}
