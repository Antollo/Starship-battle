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

using namespace std::string_literals;

class RenderSerializable;

class RenderSerializerBase
{
public:
    virtual void draw(const RenderSerializable& drawable, const sf::RenderStates& states = sf::RenderStates::Default) noexcept = 0;
    virtual void draw(const sf::VertexArray& vertexArray, const sf::RenderStates& states = sf::RenderStates::Default) noexcept = 0;
};

class RenderSerializable
{
private:
    friend class RenderSerializerBase;
    friend class RenderSerializer;
    virtual void draw(RenderSerializerBase&, sf::RenderStates states) const noexcept = 0;
};


class Object : public sf::Drawable, public RenderSerializable 
{
public:
	using ObjectId = std::int32_t;
	using ObjectsContainer = std::map<ObjectId, std::unique_ptr<Object>>;
	enum class TypeId { Invalid, PrototypeSpaceship, Spaceship, Bullet, Bot, Rock };

	/*class UpdateState {
	public:
		TypeId typeId;
		bool destroy;
		Vec2f position, linearVelocity;
		float angle, angularVelocity;
	};
	virtual const void consumeUpdateState(const UpdateState& state) const noexcept = 0;*/
	virtual const TypeId getTypeId() const { return TypeId::Invalid; };
    virtual Vec2f getCenterPosition() const = 0;
    Object() : destroy(false), id(++counter) {};
    virtual ObjectId getId() { return id; }
    virtual void process()
    {
        if (destroy) onDestroy();
    }
    bool destroy;
    static int counter;
	static ObjectsContainer objects;
    static ObjectId thisPlayerId;
    static b2World world;
    static std::random_device rd;
    static std::mt19937 mt;
    static std::uniform_real_distribution<float> rng025, rng01;
    static constexpr float worldScale = 40.f;
    static constexpr float worldLimits = 12000.f;
	virtual ~Object() { }
private:
    ObjectId id;
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept = 0;
    
	void onDestroy()
    {
		//objects.erase(getId()); Bullets represents owners;
		objects.erase(id);
        /*objects.erase(std::find_if(objects.begin(), objects.end(), [this](const auto& it) {
            return it.second.get() == this;
        }));*/
    }
};

#endif