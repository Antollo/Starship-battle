#ifndef ROCK_H_
#define ROCK_H_

#include <utility>
#include <stdexcept>
#include "Object.h"

class Rock : public Object
{
public:
    static Rock* create()
    {
        return dynamic_cast<Rock*>(objects.emplace(counter, new Rock()).first->second.get());
    }
    static Rock* create(const sf::VertexArray& newPolygon, b2Body* newBody)
    {
        return dynamic_cast<Rock*>(objects.emplace(counter, new Rock(newPolygon, newBody)).first->second.get());
    }
    static Rock* create(std::vector<Vec2f> points, float x, float y)
    {
        return dynamic_cast<Rock*>(objects.emplace(counter, new Rock(points, x, y)).first->second.get());
    }
    static Rock* create(std::vector<Vec2f> points)
    {
        return dynamic_cast<Rock*>(objects.emplace(counter, new Rock(points)).first->second.get());
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
    Vec2f getLinearVelocity() const override
    {
        return Vec2f::asVec2f(body->GetLinearVelocity()) * worldScale;
    }
    float getAngularVelocity() const override
    {
        return body->GetAngularVelocity() * 180.f / pi;
    }
    ~Rock() override
    {
        world.DestroyBody(body);
    }
    void process(float delta) override
    {
        body->GetMassData(&massData);
        setOrigin(massData.center.x * worldScale, massData.center.y * worldScale);
        setPosition(body->GetPosition().x * worldScale, body->GetPosition().y * worldScale);
        setRotation(body->GetAngle() * 180.f / pi);
        setOrigin(0, 0);
    }
private:
    static constexpr float baseDensity = 2.f;
    static constexpr float baseDensityForBorders = 8.f;
    static constexpr float randomDensity = 3.f;
    Rock()
    {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.5f;
        bodyDef.angularDamping = 0.5f;
        bodyDef.userData = this;
        bodyDef.position.x = (uniformRNG<0, 1, 1, 1>() * worldLimits * 2.f - worldLimits)/worldScale;
        bodyDef.position.y = (uniformRNG<0, 1, 1, 1>() * worldLimits * 2.f - worldLimits)/worldScale;

        body = world.CreateBody(&bodyDef);
        std::vector<Vec2f> points(n);
        float angle, radius = Object::uniformRNG<0, 1, 1, 1>() * 8.f + 6.f;
        for (auto& point : points)
        {
            angle = Object::uniformRNG<0, 1, 1, 1>() * pi * 2.f;
            point.x = std::cos(angle) * radius;
            point.y = std::sin(angle) * radius;
        }
        std::sort(points.begin(), points.end(), [](const auto& a, const auto& b){
            return std::atan2(a.y, a.x) < std::atan2(b.y, b.x);
        });
        
        b2PolygonShape shape;
        shape.Set(reinterpret_cast<b2Vec2*>(points.data()), n);

        b2FixtureDef fixtureDef;
        fixtureDef.density = randomDensity + Object::uniformRNG<0, 1, 1, 1>() * baseDensity;
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
    Rock(std::vector<Vec2f>& points, float x, float y)
    {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.5f;
        bodyDef.angularDamping = 0.5f;
        bodyDef.userData = this;
        bodyDef.position.x = x;
        bodyDef.position.y = y;

        body = world.CreateBody(&bodyDef);

        std::sort(points.begin(), points.end(), [](const auto& a, const auto& b){
            return std::atan2(a.y, a.x) < std::atan2(b.y, b.x);
        });
        
        b2PolygonShape shape;
        shape.Set(reinterpret_cast<const b2Vec2*>(points.data()), points.size());

        b2FixtureDef fixtureDef;
        fixtureDef.density = randomDensity + Object::uniformRNG<0, 1, 1, 1>() * baseDensityForBorders;
        fixtureDef.friction = 0.5f;
        fixtureDef.filter.groupIndex = 0; 
        fixtureDef.shape = &shape;
        body->CreateFixture(&fixtureDef);

        polygon.resize(points.size() + 1);
        polygon.setPrimitiveType(sf::PrimitiveType::LineStrip);
        for (std::size_t i = 0; i < points.size(); i++)
        {
            polygon[i].position = (sf::Vector2f) points[i] * worldScale;
            polygon[i].color = sf::Color::White;
        }
        polygon[points.size()] = polygon[0];
    }
    Rock(std::vector<Vec2f>& points)
    {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.5f;
        bodyDef.angularDamping = 0.5f;
        bodyDef.userData = this;
        bodyDef.position.x = 0.f;
        bodyDef.position.y = 0.f;

        body = world.CreateBody(&bodyDef);
        
        b2PolygonShape shape;
        shape.Set(reinterpret_cast<const b2Vec2*>(points.data()), points.size());

        b2FixtureDef fixtureDef;
        fixtureDef.density = randomDensity + Object::uniformRNG<0, 1, 1, 1>() * baseDensityForBorders;
        fixtureDef.friction = 0.5f;
        fixtureDef.filter.groupIndex = 0; 
        fixtureDef.shape = &shape;
        body->CreateFixture(&fixtureDef);

        polygon.resize(points.size() + 1);
        polygon.setPrimitiveType(sf::PrimitiveType::LineStrip);
        for (std::size_t i = 0; i < points.size(); i++)
        {
            polygon[i].position = (sf::Vector2f) points[i] * worldScale;
            polygon[i].color = sf::Color::White;
        }
        polygon[points.size()] = polygon[0];
    }
    Rock(const sf::VertexArray& newPolygon, b2Body* newBody)
        : body(newBody)
    {
        polygon = newPolygon;
        body->SetUserData(this);
    }
    b2Body* body;
    b2MassData massData;
    static constexpr std::size_t n = 7;
};

#endif
