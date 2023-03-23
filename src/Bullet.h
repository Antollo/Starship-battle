#ifndef BULLET_H_
#define BULLET_H_

#include "Object.h"
#include "prototypes.h"

class Spaceship;

class Bullet : public Object
{
public:
    static Bullet *create(const BulletPrototype &prototype, Vec2f position, sf::Angle angle, const Vec2f &velocity, int index)
    {
        return dynamic_cast<Bullet *>(objects.emplace(counter, new Bullet(prototype, position, angle, velocity, index)).first->second.get());
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Bullet;
    }
    Vec2f getCenterPosition() const override
    {
        return getTransform().transformPoint(proto.origin);
    }
    Vec2f getLinearVelocity() const override
    {
        return Vec2f::asVec2f(body->GetLinearVelocity()) * world::scale;
    }
    sf::Angle getAngularVelocity() const override
    {
        return sf::radians(body->GetAngularVelocity());
    }
    ~Bullet() override
    {
        world.DestroyBody(body);
    }
    void process(float delta) override
    {
        setOrigin(proto.origin);
        setPosition(Vec2f::asVec2f(body->GetPosition()) * world::scale);
        setRotation(sf::radians(body->GetAngle()));
        setOrigin({0, 0});
        destroy = destroy || body->GetLinearVelocity().LengthSquared() < minimumBulletVelocity;
    }
    static constexpr float minimumBulletVelocity = 1.f;

private:
    Bullet(const BulletPrototype &prototype, Vec2f position, sf::Angle angle, const Vec2f &velocity, int index)
        : proto(prototype)
    {
        b2BodyDef bodyDef;
        bodyDef.position = position;
        bodyDef.angle = angle.asRadians();
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.1f;
        bodyDef.angularDamping = 0.1f;
        bodyDef.bullet = true;
        bodyDef.userData = this;

        body = world.CreateBody(&bodyDef);

        b2FixtureDef fixtureDef;
        fixtureDef.density = 7.5f;
        fixtureDef.friction = 0.2f;
        fixtureDef.filter.groupIndex = index;
        fixtureDef.shape = &proto.shape;
        body->CreateFixture(&fixtureDef);

        body->SetLinearVelocity(velocity);
        process(0);
    }

    ObjectId getId() override
    {
        return -body->GetFixtureList()[0].GetFilterData().groupIndex;
    }

    void draw(RenderSerializerBase &target) const override
    {
        target.draw(proto.shapeId, getRotation(), getCenterPosition(), getLinearVelocity(), getAngularVelocity());
    }

    b2Body *body;
    Spaceship *lastContact = nullptr;
    const BulletPrototype &proto;
    friend class ContactListener;
};

#endif
