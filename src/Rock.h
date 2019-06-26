#ifndef ROCK_H_
#define ROCK_H_

#include <fstream>
#include <utility>
#include <stdexcept>
#include "Object.h"
#include "Console.h"

class Rock : public Object, public sf::Transformable
{
public:
    static Rock* create()
    {
        return dynamic_cast<Rock*>(objects.emplace(counter, new Rock()).first->second.get());
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Rock;
    }
    Vec2f getCenterPosition() const override
    {
        body->GetMassData(const_cast<b2MassData*>(&massData));
        return getTransform().transformPoint(massData.center.x * worldScale, massData.center.y * worldScale);
    }
    ~Rock() override
    {
        world.DestroyBody(body);
    }
    void process() override
    {
        body->GetMassData(&massData);
        setOrigin(massData.center.x * worldScale, massData.center.y * worldScale);
        setPosition(body->GetPosition().x * worldScale, body->GetPosition().y * worldScale);
        setRotation(body->GetAngle() * 180.f / pi);
        setOrigin(0, 0);
        Object::process();
    }
private:
    Rock()
    {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.5f;
        bodyDef.angularDamping = 0.5f;
        bodyDef.userData = this;
        bodyDef.position.x = (rng01(mt) * worldLimits * 2.f - worldLimits)/worldScale;
        bodyDef.position.y = (rng01(mt) * worldLimits * 2.f - worldLimits)/worldScale;

        body = world.CreateBody(&bodyDef);
        std::vector<Vec2f> points(n);
        float angle, radius = Object::rng01(mt) * 8 + 6;
        for (auto& point : points)
        {
            angle = Object::rng01(mt) * pi * 2.f;
            point.x = std::cos(angle) * radius;
            point.y = std::sin(angle) * radius;
        }
        std::sort(points.begin(), points.end(), [](const auto& a, const auto& b){
            return std::atan2(a.y, a.x) < std::atan2(b.y, b.x);
        });
        
        b2PolygonShape shape;
        shape.Set(reinterpret_cast<b2Vec2*>(points.data()), n);

        b2FixtureDef fixtureDef;
        fixtureDef.density = 4.f;
        fixtureDef.friction = 0.5f;
        fixtureDef.filter.groupIndex = 0; 
        fixtureDef.shape = &shape;
        body->CreateFixture(&fixtureDef);

        polygon.resize(n + 1);
        polygon.setPrimitiveType(sf::PrimitiveType::LineStrip);
        for (std::size_t i = 0; i < n; i++)
        {
            polygon[i].position = (sf::Vector2f) points[i] * worldScale;
            polygon[i].color = sf::Color::White;
        }
        polygon[n] = polygon[0];
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states);
    }
    void draw(RenderSerializerBase& target, sf::RenderStates states) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states);
    }
    sf::VertexArray polygon;
    b2Body* body;
    b2MassData massData;
    static constexpr std::size_t n = 7;
};

#endif
