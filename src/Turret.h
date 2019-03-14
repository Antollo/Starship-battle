#ifndef TURRET_H_
#define TURRET_H_

#include <fstream>
#include "Object.h"
#include "Bullet.h"

class Turret : public sf::Drawable, public sf::Transformable
{
public:
    Turret(const std::string& type)
    {
        std::ifstream file(type + ".txt");
        //TODO: Error check

        std::size_t n;
        float x, y;
        file >> n;
        polygon.resize(n + 1);
        polygon.setPrimitiveType(sf::PrimitiveType::LineStrip);
        for (std::size_t i = 0; i < n; i++)
        {
            file >> x >> y;
            polygon[i].position = sf::Vector2f(x,  y) * Object::worldScale;
            polygon[i].color = sf::Color::White;
        }
        polygon[n] = polygon[0];

        file >> x >> y;
        setOrigin(x * Object::worldScale , y * Object::worldScale);

        file >> maxAngle;

        file >> n;
        bulletShape.resize(n);
        for (auto& point: bulletShape)
            file >> point.x >> point.y;
        file >> bulletVelocity >> accuracy >> penetrationAngle;
        rng.param(std::uniform_real_distribution<float>::param_type(-accuracy, accuracy));
    }
    void setRotation(const float& angle)
    {
        sf::Transformable::setRotation(std::min(std::max(angle, - maxAngle), maxAngle));
    }
    void shoot(const sf::Transform& transform, const float& angle, const Vec2f& velocity, const int& index)
    {
        float rotatedAngle = angle + getRotation() / 180.f * pi + rng(Object::mt);
        Bullet::create(bulletShape, Vec2f::asVec2f(transform.transformPoint(getPosition()) / Object::worldScale), rotatedAngle, penetrationAngle, {cosf(rotatedAngle) * bulletVelocity + velocity.x, sinf(rotatedAngle) * bulletVelocity + velocity.y}, index);
    }
private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states);
    }
    sf::VertexArray polygon;
    std::vector<Vec2f> bulletShape;
    float bulletVelocity, maxAngle, accuracy, penetrationAngle;
    std::uniform_real_distribution<float> rng;
};

#endif
