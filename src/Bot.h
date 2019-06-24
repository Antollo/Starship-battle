#ifndef BOT_H_
#define BOT_H_

#include <limits>
#include "Spaceship.h"

class Bot : public Spaceship
{
public:
    static Bot* create(const std::string& type)
    {
        if (type.empty()) throw std::runtime_error("Type of spaceship was empty.");
        return dynamic_cast<Bot*>(objects.emplace(counter, new Bot(type)).first->second.get());
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Bot;
    }
    void process() override
    {
        Vec2f newAimCoords{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        for (const auto& object : objects)
        {
            if ((*object.second).getTypeId() == Object::TypeId::Spaceship)
            {
                if (getCenterPosition().getSquaredDistance(object.second->getCenterPosition())
                < getCenterPosition().getSquaredDistance(newAimCoords))
                    newAimCoords = object.second->getCenterPosition();
            }
        }

        /*newAimCoords = std::min_element(objects.begin(), objects.end(), [this](const auto& a, const auto& b) {
            //if (a.second->getTypeId() == Object::TypeId::Spaceship && b.second->getTypeId() != Object::TypeId::Spaceship)
            //    return true;
            return getCenterPosition().getSquaredDistance(a.second->getCenterPosition()) < getCenterPosition().getSquaredDistance(b.second->getCenterPosition());
        })->second->getCenterPosition();*/

        if (newAimCoords != Vec2f{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()})
        {
            aim = true;
            aimCoords = newAimCoords;
            float aimAngle = std::atan2f(aimCoords.y - getCenterPosition().y, aimCoords.x - getCenterPosition().x) - body->GetAngle();
            while (aimAngle > pi) aimAngle -= 2.f * pi;
            while (aimAngle < -pi) aimAngle += 2.f * pi;

            if (getCenterPosition().getSquaredDistance(newAimCoords) < 80000.f * 80000.f && aimAngle > -0.5f && aimAngle < 0.5f)
                shoot = true;
            else
                shoot = false;

            if (aimAngle < -0.7f)
            {
                right = false;
                left = true;
                forward = false;
            }
            else if (aimAngle > 0.7f)
            {
                right = true;
                left = false;
                forward = false;
            }
            else
            {
                right = false;
                left = false;
                if (aimAngle > -0.5f && aimAngle < 0.5f) forward = true;
            }
        }
        else
        {
            aim = false;
            shoot = false;
            right = false;
            left = false;
            forward = false;
        }
        Spaceship::process();
    }
    ~Bot() override
    {
        //create(t, pID);
    }
private:
    Bot(const std::string& type) : Spaceship(type)
    { }
};

#endif /* !BOT_H_ */
