#ifndef TURRET_H_
#define TURRET_H_

#include "Object.h"
#include "Bullet.h"

class Turret : public sf::Drawable, public sf::Transformable, public RenderSerializable 
{
public:
    Turret(const std::string& type)
    {
        json jsonObject = resourceManager::getJSON(type);

        std::vector<Vec2f> points = jsonObject["points"].get<std::vector<Vec2f>>();
        std::size_t n = points.size();
        
        polygon.resize(n + 1);
        polygon.setPrimitiveType(sf::PrimitiveType::LineStrip);
        for (std::size_t i = 0; i < n; i++)
        {
            polygon[i].position = (sf::Vector2f) points[i] * Object::worldScale;
            polygon[i].color = sf::Color::White;
        }
        polygon[n] = polygon[0];

        setOrigin(jsonObject["origin"].get<Vec2f>().x * Object::worldScale , jsonObject["origin"].get<Vec2f>().y * Object::worldScale);

        maxAngle = jsonObject["maxAngle"].get<float>();

        bulletShape = jsonObject["bulletShape"].get<std::vector<Vec2f>>();

        bulletVelocity = jsonObject["bulletVelocity"].get<float>();
        accuracy = jsonObject["accuracy"].get<float>();
        damage = jsonObject["damage"].get<float>();
        penetration = jsonObject["penetration"].get<float>();

        rng.param(std::normal_distribution<float>::param_type(0, accuracy / 1.3f));
    }
    bool setRotation(const float& angle)
    {
        sf::Transformable::setRotation(std::min(std::max(angle, - maxAngle), maxAngle));
        return angle > - maxAngle && angle < maxAngle;
    }
    void shoot(const sf::Transform& transform, const float& angle, const Vec2f& velocity, const int& index)
    {
        float rotatedAngle = angle + getRotation() / 180.f * pi + rng(Object::mt);
        Bullet::create(bulletShape, Vec2f::asVec2f(transform.transformPoint(getPosition()) / Object::worldScale), rotatedAngle, penetration, damage, {cosf(rotatedAngle) * bulletVelocity + velocity.x, sinf(rotatedAngle) * bulletVelocity + velocity.y}, index);
    }
private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states);
    }
    void draw(RenderSerializerBase& target, sf::RenderStates states, const Vec2f &position, const Vec2f &linearVelocity, float angularVelocity) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states, position, linearVelocity, angularVelocity);
    }
    sf::VertexArray polygon;
    std::vector<Vec2f> bulletShape;
    float bulletVelocity, maxAngle, accuracy, penetration, damage;
    std::normal_distribution<float> rng;
};

#endif
