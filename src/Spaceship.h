#ifndef SPACESHIP_H_
#define SPACESHIP_H_

#include <utility>
#include <stdexcept>
#include "Object.h"
#include "Turret.h"
#include "Rock.h"
#include "Shield.h"

class Spaceship : public Object
{
public:
    static Spaceship *create(const std::string &type, const std::wstring &playerId, Object::ObjectId id = 0)
    {
        if (type.empty())
            throw std::runtime_error("Type of spaceship was empty.");
        if (playerId.empty())
            throw std::runtime_error("Commander ID was empty.");
        auto spaceship = new Spaceship(type, playerId, id);
        objects.emplace(spaceship->getId(), spaceship);
        return spaceship;
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Spaceship;
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
    float getReloadState()
    {
        return clock.getElapsedTime().asSeconds() / *reloadIt;
    }
    bool getAimState()
    {
        return aimState;
    }
    float getMaxHp() const
    {
        return proto.hp;
    }
    float getHp() const
    {
        return hp;
    }
    float getArmor() const
    {
        return proto.armor;
    }
    void inceaseHp(float value)
    {
        hp += value;
    }
    void increaseArmor(float value)
    {
        // TODO
        // armor += value;
    }
    const std::wstring &getPlayerId() const
    {
        return playerId;
    }
    const std::vector<std::pair<Vec2f, Vec2f>> &getEdges() const
    {
        // TODO: store in prototype 
        static std::vector<std::pair<Vec2f, Vec2f>> ret;
        ret.resize(proto.points.size() - 1);
        for (std::size_t i = 0; i < proto.points.size() - 1; i++)
        {
            ret[i].first = Vec2f::asVec2f(getTransform().transformPoint(proto.points[i]) / world::scale);
            ret[i].second = Vec2f::asVec2f(getTransform().transformPoint(proto.points[i + 1]) / world::scale);
        }
        return ret;
    }
    ~Spaceship() override
    {
        body->SetUserData(nullptr);
        decltype(Object::objects)::iterator it, jt;
        for (auto it = Object::objects.begin(); it != Object::objects.end();)
        {
            jt = it++;
            if (jt->second->getId() == getId())
                jt->second->destroy = true;
        }

        for (const auto &id : shields)
        {
            if (Object::objects.count(id))
                Object::objects[id]->destroy = true;
        }

        Rock::create(proto.shapeId, body);
    }
    void process(float delta) override
    {
        setOrigin(proto.origin);
        setPosition(Vec2f::asVec2f(body->GetPosition()) * world::scale);
        setRotation(sf::radians(body->GetAngle()));
        setOrigin({0, 0});
        if (forward)
            onForward();
        if (left)
            onLeft();
        if (right)
            onRight();
        onAim(delta);
        if (shoot)
            onShoot();
        if (hp < 0.f)
            destroy = true;
    }
    bool forward, left, right, shoot;
    Vec2f aimCoords;

private:
    Spaceship(const std::string &type, const std::wstring &newPlayerId = L"AutomatedPilot-" + std::to_wstring(counter + 1), ObjectId objectId = 0)
        : Spaceship(resourceManager::getSpaceshipPrototype(type), newPlayerId, objectId) {}

    Spaceship(const SpaceshipPrototype &prototype, const std::wstring &newPlayerId = L"AutomatedPilot-" + std::to_wstring(counter + 1), ObjectId objectId = 0);

    void draw(RenderSerializerBase &target) const override
    {
        Vec2f myLinearVelocity = getLinearVelocity();
        sf::Angle myRotation = getRotation();
        sf::Angle myAngularVelocity = getAngularVelocity();
        target.draw(proto.shapeId, myRotation, getCenterPosition(), myLinearVelocity, myAngularVelocity);
        for (const Turret &turret : turrets)
            turret.draw(target, myRotation, getTransform(), myLinearVelocity, myAngularVelocity);
    }
    void onForward() noexcept
    {
        body->ApplyForceToCenter(b2Vec2(std::cos(body->GetAngle()) * proto.force, std::sin(body->GetAngle()) * proto.force), true);
    }
    void onLeft() noexcept
    {
        body->ApplyTorque(-proto.torque, true);
    }
    void onRight() noexcept
    {
        body->ApplyTorque(proto.torque, true);
    }
    void onAim(float delta) noexcept
    {
        sf::Vector2f targetRelativeToTurret;
        aimState = false;
        for (Turret &turret : turrets)
        {
            targetRelativeToTurret = getInverseTransform().transformPoint(aimCoords) - turret.getPosition();

            float dest = std::atan2(targetRelativeToTurret.y, targetRelativeToTurret.x);
            aimState |= turret.setRotation(sf::radians(dest), delta);
        }
    }
    virtual void onShoot() noexcept
    {
        if (clock.getElapsedTime().asSeconds() > *reloadIt)
        {
            for (Turret &turret : turrets)
                turret.shoot(getTransform(), getRotation(), Vec2f::asVec2f(body->GetLinearVelocity()), -getId());
            clock.restart();
            reloadIt++;
            if (reloadIt == proto.reload.end())
                reloadIt = proto.reload.begin();
        }
    }

    const SpaceshipPrototype &proto;
    std::wstring playerId;
    b2Body *body;
    float hp;
    std::vector<float>::const_iterator reloadIt;
    bool aimState;
    std::vector<Turret> turrets;
    std::vector<Object::ObjectId> shields;
    sf::Clock clock;
    friend class Bot;
    friend class ContactListener;
    friend class RenderSerializer;
};

#endif
