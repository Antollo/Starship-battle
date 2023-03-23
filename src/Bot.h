#ifndef BOT_H_
#define BOT_H_

#include <limits>
#include <set>
#include "Spaceship.h"

class Bot : public Spaceship
{
public:
    static Bot *create(const std::string &type)
    {
        if (type.empty())
            throw std::runtime_error("Type of spaceship was empty.");
        auto bot = new Bot(type);
        objects.emplace(bot->getId(), bot);
        return bot;
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Bot;
    }
    void process(float delta) override
    {
        Vec2f newAimCoords{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        for (const auto &targetId : targets)
        {
            if (objects.count(targetId) == 0)
                continue;
            if (getCenterPosition().getSquaredDistance(objects[targetId]->getCenterPosition()) < getCenterPosition().getSquaredDistance(newAimCoords))
                newAimCoords = objects[targetId]->getCenterPosition();
        }

        if (newAimCoords != Vec2f{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()})
        {
            aimCoords = newAimCoords;
            float aimAngle = sf::radians(std::atan2(aimCoords.y - getCenterPosition().y, aimCoords.x - getCenterPosition().x) - body->GetAngle()).wrapSigned().asRadians();

            if (getCenterPosition().getSquaredDistance(newAimCoords) < 100000.f * 80000.f && aimAngle > -0.6f && aimAngle < 0.6f)
                shoot = true;
            else
                shoot = false;

            if (aimAngle < -0.6f)
            {
                right = false;
                left = true;
                forward = false;
            }
            else if (aimAngle > 0.6f)
            {
                right = true;
                left = false;
                forward = false;
            }
            else
            {
                right = false;
                left = false;
                if (aimAngle > -0.5f && aimAngle < 0.5f)
                    forward = true;
            }
        }
        else
        {
            shoot = false;
            right = false;
            left = false;
            forward = false;
        }
        Spaceship::process(delta);
    }
    void target(const Object::ObjectId &id)
    {
        targets.insert(id);
    }
    static void allTarget(const Object::ObjectId &id)
    {
        for (auto &obj : objects)
            if (obj.second->getTypeId() == Object::TypeId::Bot && obj.second->getId() != id)
                dynamic_cast<Bot &>(*obj.second).target(id);
    }

private:
    std::set<Object::ObjectId> targets;
    Bot(const std::string &type) : Spaceship(type)
    {
    }
    void onShoot() noexcept override
    {
        if (clock.getElapsedTime().asSeconds() > *reloadIt)
            if (Rng::uniform() < 0.3f)
                clock.restart();
            else
                Spaceship::onShoot();
    }
};

#endif /* !BOT_H_ */
