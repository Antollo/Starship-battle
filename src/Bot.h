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
        objects.emplace_back(new Bot(type));
        return dynamic_cast<Bot*>(objects.back().get());
    }
    const Object::typeId getTypeId() const noexcept override
    {
        return Object::typeId::Bot;
    }
    void process() override
    {
        Vec2f newAimCoords{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        for (const auto& object : objects)
        {
            if ((*object).getTypeId() == Object::typeId::Spaceship)
            {
                if (Vec2f::asVec2f(getCenterPosition()).squaredDistance(Vec2f::asVec2f(dynamic_cast<Spaceship*>(object.get())->getCenterPosition()))
                < newAimCoords.squaredDistance(Vec2f::asVec2f(dynamic_cast<Spaceship*>(object.get())->getCenterPosition())))
                    newAimCoords = Vec2f::asVec2f(dynamic_cast<Spaceship*>(object.get())->getCenterPosition());
            }
        }
        if (newAimCoords != Vec2f{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()})
        {
            aim = true;
            if (Vec2f::asVec2f(getCenterPosition()).squaredDistance(newAimCoords) < 120000.f * 120000.f) shoot = true;
            else shoot = false;
            aimCoords = newAimCoords;
            float aimAngle = std::atan2f(aimCoords.y - getCenterPosition().y, aimCoords.x - getCenterPosition().x) - body->GetAngle();
            while (aimAngle > pi) aimAngle -= 2.f * pi;
            while (aimAngle < -pi) aimAngle += 2.f * pi;
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
    Bot(const std::string& type, const std::wstring& playerID = L"Bot" + std::to_wstring(counter)) : Spaceship(type, playerID)
    { }
};

#endif /* !BOT_H_ */
