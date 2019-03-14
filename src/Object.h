#ifndef OBJECT_H_
#define OBJECT_H_

#include <memory>
#include <deque>
#include <list>
#include <algorithm>
#include <iostream>
#include <random>
#include <Box2D\Box2D.h>
#include <SFML\Graphics.hpp>

constexpr float pi = 3.14159265358979323846f;

class Vec2f
{
public:
    float x, y;
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
    float squaredDistance(const Vec2f& vec) const
    {
        return (x-vec.x)*(x-vec.x) + (y-vec.y)*(y-vec.y);
    }
};


class Console;
class Spaceship;
class ParticleSystem;

class Object : public sf::Drawable
{
public:
	using ObjectsContainer = std::list<std::unique_ptr<Object>>;
    enum class typeId { Invalid, Console, PrototypeSpaceship, Spaceship, Bullet, Cursor, ParticicleSystem, Bot, Rock };
    virtual const typeId getTypeId() const noexcept = 0;
    Object() : destroy(false) {};
    virtual ~Object() {};
    virtual void process()
    {
        if (destroy) onDestroy();
    }
    bool destroy;
    static int counter; //Physics
	static ObjectsContainer objects;
    static Console* console;
    static Spaceship* spaceship;
    static ParticleSystem* particleSystem;
    static b2World world;
    static std::random_device rd;
    static std::mt19937 mt;
    static std::uniform_real_distribution<float> rng025, rng01;
    static constexpr float worldScale = 40.f;
    static constexpr float worldLimits = 12000.f;
private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept = 0;
	void onDestroy()
    {
        objects.erase(std::find_if (objects.cbegin(), objects.cend(), [this](const std::unique_ptr<Object>& ptr) {
            return ptr.get() == this;
        }));
        if (reinterpret_cast<Object*>(console) == this) console = nullptr;
		if (reinterpret_cast<Object*>(spaceship) == this) spaceship = nullptr;
        if (reinterpret_cast<Object*>(particleSystem) == this) particleSystem = nullptr;
    }
};

#endif