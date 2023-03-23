#ifndef ROCK_H_
#define ROCK_H_

#include <utility>
#include <stdexcept>
#include "Object.h"
#include "ServerShape.h"

class Rock : public Object
{
public:
    static Rock *create()
    {
        return dynamic_cast<Rock *>(objects.emplace(counter, new Rock()).first->second.get());
    }
    static Rock *create(Shape::IdType newShape, b2Body *newBody)
    {
        return dynamic_cast<Rock *>(objects.emplace(counter, new Rock(newShape, newBody)).first->second.get());
    }
    static Rock *create(const std::vector<Vec2f> &points, const Vec2f &position)
    {
        return dynamic_cast<Rock *>(objects.emplace(counter, new Rock(points, position)).first->second.get());
    }
    static Rock *create(const std::vector<Vec2f> &points)
    {
        return dynamic_cast<Rock *>(objects.emplace(counter, new Rock(points)).first->second.get());
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Rock;
    }
    Vec2f getCenterPosition() const override
    {
        body->GetMassData(const_cast<b2MassData *>(&massData));
        return getTransform().transformPoint(Vec2f::asVec2f(massData.center) * world::scale);
    }
    Vec2f getLinearVelocity() const override
    {
        return Vec2f::asVec2f(body->GetLinearVelocity()) * world::scale;
    }
    sf::Angle getAngularVelocity() const override
    {
        return sf::radians(body->GetAngularVelocity());
    }
    ~Rock() override
    {
        world.DestroyBody(body);
    }
    void process(float delta) override
    {
        body->GetMassData(&massData);
        setOrigin(Vec2f::asVec2f(massData.center) * world::scale);
        setPosition(Vec2f::asVec2f(body->GetPosition()) * world::scale);
        setRotation(sf::radians(body->GetAngle()));
        setOrigin({0, 0});
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
        bodyDef.position = (Rng::uniform2d() * world::limits * 2.f - world::limits) / world::scale;

        body = world.CreateBody(&bodyDef);
        std::vector<Vec2f> points(n);
        float radius = Rng::uniform() * 8.f + 6.f;
        for (auto &point : points)
        {
            float angle = Rng::uniform() * pi * 2.f;
            point.x = std::cos(angle) * radius;
            point.y = std::sin(angle) * radius;
        }
        std::sort(points.begin(), points.end(), [](const auto &a, const auto &b)
                  { return std::atan2(a.y, a.x) < std::atan2(b.y, b.x); });

        b2PolygonShape shape;
        shape.Set(reinterpret_cast<b2Vec2 *>(points.data()), points.size());

        b2FixtureDef fixtureDef;
        fixtureDef.density = randomDensity + Rng::uniform() * baseDensity;
        fixtureDef.friction = 0.5f;
        fixtureDef.filter.groupIndex = 0;
        fixtureDef.shape = &shape;
        fixtureDef.thickShape = true;
        body->CreateFixture(&fixtureDef);
        body->GetMassData(&massData);

        points.resize(points.size() + 1);
        for (auto &point : points)
            point *= world::scale;
        points.back() = points.front();

        shapeId = ServerShape::setShape(points, Vec2f::asVec2f(massData.center) * world::scale);
    }
    Rock(std::vector<Vec2f> points, const Vec2f &position)
    {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.5f;
        bodyDef.angularDamping = 0.5f;
        bodyDef.userData = this;
        bodyDef.position = position;

        body = world.CreateBody(&bodyDef);

        std::sort(points.begin(), points.end(), [](const auto &a, const auto &b)
                  { return std::atan2(a.y, a.x) < std::atan2(b.y, b.x); });

        b2PolygonShape shape;
        shape.Set(reinterpret_cast<const b2Vec2 *>(points.data()), points.size());

        b2FixtureDef fixtureDef;
        fixtureDef.density = randomDensity + Rng::uniform() * baseDensityForBorders;
        fixtureDef.friction = 0.5f;
        fixtureDef.filter.groupIndex = 0;
        fixtureDef.shape = &shape;
        body->CreateFixture(&fixtureDef);
        body->GetMassData(&massData);

        points.resize(points.size() + 1);
        for (auto &point : points)
            point *= world::scale;
        points.back() = points.front();

        shapeId = ServerShape::setShape(points, Vec2f::asVec2f(massData.center) * world::scale);
    }
    Rock(std::vector<Vec2f> points)
    {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.5f;
        bodyDef.angularDamping = 0.5f;
        bodyDef.userData = this;
        bodyDef.position = {0.f, 0.f};

        body = world.CreateBody(&bodyDef);

        b2PolygonShape shape;
        shape.Set(reinterpret_cast<const b2Vec2 *>(points.data()), points.size());

        b2FixtureDef fixtureDef;
        fixtureDef.density = randomDensity + Rng::uniform() * baseDensityForBorders;
        fixtureDef.friction = 0.5f;
        fixtureDef.filter.groupIndex = 0;
        fixtureDef.shape = &shape;
        body->CreateFixture(&fixtureDef);
        body->GetMassData(&massData);

        points.resize(points.size() + 1);
        for (auto &point : points)
            point *= world::scale;
        points.back() = points.front();

        shapeId = ServerShape::setShape(points, Vec2f::asVec2f(massData.center) * world::scale);
    }
    Rock(Shape::IdType newShape, b2Body *newBody)
        : body(newBody)
    {
        shapeId = newShape;
        body->SetUserData(this);
    }

    void draw(RenderSerializerBase &target) const override
    {
        target.draw(shapeId, getRotation(), getCenterPosition(), getLinearVelocity(), getAngularVelocity());
    }

    b2Body *body;
    b2MassData massData;
    Shape::IdType shapeId;
    static constexpr std::size_t n = 7;
};

#endif
