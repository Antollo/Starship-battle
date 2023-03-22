#ifndef TURRET_H_
#define TURRET_H_

#include "Object.h"
#include "Bullet.h"

class Turret : public sf::Transformable, public RenderSerializable 
{
public:
    Turret(const std::string& type)
    {
        json jsonObject = resourceManager::getJSON(type);

        setOrigin(jsonObject["origin"].get<Vec2f>().x * Object::worldScale , jsonObject["origin"].get<Vec2f>().y * Object::worldScale);

        std::vector<Vec2f> points = jsonObject["points"].get<std::vector<Vec2f>>();
        std::size_t n = points.size();

        points.resize(n + 1);
        for (size_t i = 0; i < n; i++)
            points[i] = points[i] * Object::worldScale;
        points[n] = points[0];

        shape = ServerShape::setShape(points, getOrigin());

        maxAngle = jsonObject["maxAngle"].get<float>();

        bulletShape = jsonObject["bulletShape"].get<std::vector<Vec2f>>();

        bulletVelocity = jsonObject["bulletVelocity"].get<float>();
        accuracy = jsonObject["accuracy"].get<float>();
        damage = jsonObject["damage"].get<float>();
        penetration = jsonObject["penetration"].get<float>();

        rng.setParams(0, accuracy / 1.3f);
    }
    bool setRotation(float dest, float delta)
    {
        dest = piPi(dest);
        float last = piPi(getRotation());
        float diff = piPi(dest - last);

        if (std::abs(diff) > 10.f)
        {
            if (maxAngle < 180.f)
                diff = dest - last;

            if (diff > 0)
                dest = last + delta * 360.f;
            else
                dest = last - delta * 360.f;
        }
        sf::Transformable::setRotation(std::min(std::max(dest, - maxAngle), maxAngle));
        return dest > - maxAngle && dest < maxAngle;
    }
    void shoot(const sf::Transform& transform, const float& angle, const Vec2f& velocity, int index)
    {
        float rotatedAngle = angle + getRotation() / 180.f * pi + rng();
        Bullet::create(bulletShape, Vec2f::asVec2f(transform.transformPoint(getPosition()) / Object::worldScale), rotatedAngle, penetration, damage, {cosf(rotatedAngle) * bulletVelocity + velocity.x, sinf(rotatedAngle) * bulletVelocity + velocity.y}, index);
    }
private:
    void draw(RenderSerializerBase& target, float rotation, const sf::Transform &transform, const Vec2f &linearVelocity, float angularVelocity) const
    {
        target.draw(shape, getRotation() + rotation, transform.transformPoint(getPosition()), linearVelocity, angularVelocity);
    }

    void draw(RenderSerializerBase &renderSerializer) const override { }
    
    inline float piPi(float x){
        x = std::fmod(x + 180.f, 360.f);
        if (x < 0)
            x += 360.f;
        return x - 180.f;
    }
    std::vector<Vec2f> bulletShape;
    float bulletVelocity, maxAngle, accuracy, penetration, damage;
    Rng::Normal rng;

    friend class Spaceship;
};

#endif
