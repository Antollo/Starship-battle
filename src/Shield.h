#ifndef SHIELD_H_
#define SHIELD_H_

#include <utility>
#include <stdexcept>
#include "Object.h"

class Shield : public Object
{
public:
    static Shield *create(const ShieldPrototype &prototype, int index, b2Vec2 position)
    {
        return dynamic_cast<Shield *>(objects.emplace(counter, new Shield(prototype, index, position)).first->second.get());
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Shield;
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
    ~Shield() override
    {
        if (joint)
            world.DestroyJoint(joint);

        Rock::create(proto.shapeId, body);
    }
    void process(float delta) override
    {
        setOrigin(proto.origin);
        setPosition(Vec2f::asVec2f(body->GetPosition()) * world::scale);
        setRotation(sf::radians(body->GetAngle()));
        setOrigin({0, 0});
        // joint->SetMotorSpeed(-100.f * (piPi(joint->GetJointAngle())));
        // std::cout<<piPi(joint->GetJointAngle())<<std::endl;
    }
    void connect(b2Body *shipBody, float x, float y)
    {
        b2WeldJointDef jointDef;
        jointDef.bodyA = shipBody;
        jointDef.bodyB = body;
        jointDef.localAnchorB = body->GetLocalCenter();
        jointDef.localAnchorB.x += x / 2.f;
        jointDef.localAnchorB.y += y / 2.f;

        jointDef.localAnchorA = shipBody->GetLocalCenter();
        jointDef.localAnchorA.x -= x / 2.f;
        jointDef.localAnchorA.y -= y / 2.f;

        jointDef.collideConnected = true;
        // jointDef.referenceAngle = 1;
        jointDef.frequencyHz = 8;
        jointDef.dampingRatio = 0.8;

        // jointDef.enableMotor = true;
        // jointDef.maxMotorTorque = 100;
        // jointDef
        // jointDef.motorSpeed = 100;
        joint = (b2WeldJoint *)world.CreateJoint(&jointDef);
    }

private:
    Shield(const ShieldPrototype &prototype, int index, b2Vec2 position)
        : proto(prototype)
    {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.01f;
        bodyDef.angularDamping = 1.f;
        bodyDef.userData = this;
        bodyDef.position = position;
        bodyDef.bullet = true;

        body = world.CreateBody(&bodyDef);

        b2FixtureDef fixtureDef;
        fixtureDef.density = 1.f;
        fixtureDef.friction = 0.1f;
        fixtureDef.filter.groupIndex = index;
        fixtureDef.shape = &proto.shape;
        body->CreateFixture(&fixtureDef);

        joint = nullptr;
    }
    void draw(RenderSerializerBase &target) const noexcept override
    {
        target.draw(proto.shapeId, getRotation(), getCenterPosition(), getLinearVelocity(), getAngularVelocity());
    }
    friend class Spaceship;
    const ShieldPrototype &proto;
    b2Body *body;
    b2WeldJoint *joint;
};

#endif
