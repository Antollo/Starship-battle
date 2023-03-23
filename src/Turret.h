#ifndef TURRET_H_
#define TURRET_H_

#include "Object.h"
#include "Bullet.h"
#include "prototypes.h"
#include "resourceManager.h"

class Turret : public sf::Transformable, public RenderSerializable
{
public:
    Turret(const TurretPrototype &prototype)
        : proto(prototype)
    {
        setOrigin(proto.origin);
    }
    bool setRotation(sf::Angle destAngle, float delta)
    {
        float dest = destAngle.wrapSigned().asDegrees();
        float last = getRotation().wrapSigned().asDegrees();
        float diff = sf::degrees(dest - last).wrapSigned().asDegrees();

        if (std::abs(diff) > 20.f)
        {
            if (diff > 0)
                dest = last + delta * 540.f;
            else
                dest = last - delta * 540.f;
        }

        sf::Transformable::setRotation(sf::degrees(std::min(std::max(dest, -proto.maxAngle.asDegrees()), proto.maxAngle.asDegrees())));
        return dest > -proto.maxAngle.asDegrees() && dest < proto.maxAngle.asDegrees();
    }
    void shoot(const sf::Transform &transform, sf::Angle angle, const Vec2f &velocity, int index)
    {
        sf::Angle rotatedAngle = sf::radians(angle.asRadians() + getRotation().asRadians() + proto.rng());
        Bullet::create(proto.bulletPrototype, Vec2f::asVec2f(transform.transformPoint(getPosition()) / world::scale), rotatedAngle, Vec2f::asVec2f(sf::Vector2f(proto.bulletVelocity, 0.f).rotatedBy(rotatedAngle)), index);
    }

private:
    void draw(RenderSerializerBase &target, sf::Angle rotation, const sf::Transform &transform, const Vec2f &linearVelocity, sf::Angle angularVelocity) const
    {
        target.draw(proto.shapeId, getRotation() + rotation, transform.transformPoint(getPosition()), linearVelocity, angularVelocity);
    }

    void draw(RenderSerializerBase &renderSerializer) const override {}

    const TurretPrototype &proto;

    friend class Spaceship;
};

#endif
