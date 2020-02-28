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
    static Spaceship* create(const std::string& type, const std::wstring& playerId)
    {
        if (type.empty()) throw std::runtime_error("Type of spaceship was empty.");
        if (playerId.empty()) throw std::runtime_error("Commander ID was empty.");
        auto spaceship = new Spaceship(type, playerId);
        objects.emplace(spaceship->getId(), spaceship);
        return spaceship;
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Spaceship;
    }
    Vec2f getCenterPosition() const override
    {
        body->GetMassData(const_cast<b2MassData*>(&massData));
        return getTransform().transformPoint(massData.center.x * worldScale, massData.center.y * worldScale);
    }
    Vec2f getLinearVelocity() const override
    {
        return Vec2f::asVec2f(body->GetLinearVelocity()) * worldScale;
    }
    float getAngularVelocity() const override
    {
        return body->GetAngularVelocity() * 180.f / pi;
    }
    float getReloadState()
    {
        return clock.getElapsedTime().asSeconds() / reload;
    }
    bool getAimState()
    {
        return aimState;
    }
    const float& getMaxHp()
    {
        return maxHp;
    }
    const float& getHp()
    {
        return hp;
    }
    const std::wstring& getPlayerId()
    {
        return playerId;
    }
    std::vector<std::pair<Vec2f, Vec2f>> getEdges()
    {
        std::vector<std::pair<Vec2f, Vec2f>> ret(polygon.getVertexCount() - 1);
        for (std::size_t i = 0; i < polygon.getVertexCount() - 1; i++)
        {
            ret[i].first = Vec2f::asVec2f(getTransform().transformPoint(polygon[i].position) / worldScale);
            ret[i].second = Vec2f::asVec2f(getTransform().transformPoint(polygon[i + 1].position) / worldScale);
        }
        return ret;
    }
    ~Spaceship() override
    {
        //(*Object::console) << Spaceship::playerId
        //<< L" was warped to HQ\n";
		//if (spaceship == this) spaceship = nullptr;
		//if (getId() == thisPlayerId) thisPlayerId = -1;

        //world.DestroyBody(body);
        body->SetUserData(nullptr);
        decltype(Object::objects)::iterator it, jt;
        for (auto it = Object::objects.begin(); it != Object::objects.end();)
        {
                jt = it++;
                if (jt->second->getId() == getId())
                    jt->second->destroy = true;
        }

        for (const auto& id: shields)
        {
            if(Object::objects.count(id))
                Object::objects[id]->destroy = true;
        }
        
        Rock::create(polygon, body);
    }
    void process() override
    {
        body->GetMassData(&massData);
        setOrigin(massData.center.x * worldScale, massData.center.y * worldScale);
        setPosition(body->GetPosition().x * worldScale, body->GetPosition().y * worldScale);
        setRotation(body->GetAngle() * 180.f / pi);
        setOrigin(0, 0);
        if (forward) onForward();
        if (left) onLeft();
        if (right) onRight();
        if (aim) onAim();
        if (shoot) onShoot();
        if (hp < 0.f) destroy = true;
    }
    bool forward, left, right, aim, shoot;
    Vec2f aimCoords;
private:
    Spaceship(const std::string& type, const std::wstring& newPlayerId = L"AutomatedPilot-" + std::to_wstring(counter + 1)) : forward(false), left(false), right(false), aim(false), shoot(false), playerId(newPlayerId)
    {
        json jsonObject = resourceManager::getJSON(type);

        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = jsonObject["linearDamping"].get<float>();
        bodyDef.angularDamping = jsonObject["angularDamping"].get<float>();
        bodyDef.userData = this;
        bodyDef.position.x = (rng01(mt) * worldLimits * 2.f - worldLimits)/worldScale;
        bodyDef.position.y = (rng01(mt) * worldLimits * 2.f - worldLimits)/worldScale;
        bodyDef.angle = rng01(mt) * 2.f * pi;

        body = world.CreateBody(&bodyDef);

        std::vector<Vec2f> points = jsonObject["points"].get<std::vector<Vec2f>>();// (n);
        std::size_t n = points.size();

        b2PolygonShape shape;
        shape.Set(reinterpret_cast<b2Vec2*>(points.data()), n);

        b2FixtureDef fixtureDef;
        fixtureDef.density = jsonObject["density"].get<float>();
        fixtureDef.friction = jsonObject["friction"].get<float>();
        fixtureDef.filter.groupIndex = -getId(); 
        fixtureDef.shape = &shape;
        body->CreateFixture(&fixtureDef);

        polygon.resize(n + 1);
        polygon.setPrimitiveType(sf::PrimitiveType::LineStrip);
        for (std::size_t i = 0; i < n; i++)
        {
            polygon[i].position = (sf::Vector2f) points[i] * worldScale;
            polygon[i].color = sf::Color::White;
        }
        polygon[n] = polygon[0];

        force = jsonObject["force"].get<float>();
        torque = jsonObject["torque"].get<float>();
        reload = jsonObject["reload"].get<float>();
        maxHp = hp = jsonObject["hp"].get<float>();
        armor = jsonObject["armor"].get<float>();
        /*std::string str;
        float x, y;
        file >> n;
        while (n--)
        {
            file >> str >> x >> y;
            turrets.emplace_back(str);
            turrets.back().setPosition(x * worldScale, y * worldScale);
        }*/
        for (auto& turret : jsonObject["turrets"])
        {
            turrets.emplace_back(turret["type"].get<std::string>());
            turrets.back().setPosition(turret["x"].get<float>() * worldScale, turret["y"].get<float>() * worldScale);
        }

        Shield *shield;
        const auto& shieldsJson = jsonObject["shields"];

        for (const auto& shieldJson : shieldsJson)
        {
            shield = Shield::create(shieldJson["points"].get<std::vector<Vec2f>>(), -getId());
            shield->connect(body, shieldJson["x"].get<float>(), shieldJson["y"].get<float>());
            shields.push_back(shield->getId());
        }
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states);
        for (const Turret& turret : turrets)
            target.draw(turret, states);
    }
    void draw(RenderSerializerBase& target, sf::RenderStates states, const Vec2f &position, const Vec2f &linearVelocity, float angularVelocity) const noexcept override
    {
        states.transform *= getTransform();
        Vec2f myLinearVelocity = getLinearVelocity();
        Vec2f myPosition = getCenterPosition();
        float myAngularVelocity = getAngularVelocity();
        target.draw(polygon, states, myPosition, myLinearVelocity, myAngularVelocity);
        for (const Turret& turret : turrets)
            target.draw(turret, states, myPosition, myLinearVelocity, myAngularVelocity);
        for (const ObjectId & objectId : shields)
            dynamic_cast<const Shield*>(objects[objectId].get())->drawS(target, sf::RenderStates::Default, myPosition, myLinearVelocity, myAngularVelocity);
    }
    void onForward() noexcept
    {
        body->ApplyForceToCenter(b2Vec2(cosf(body->GetAngle()) * force, sinf(body->GetAngle()) * force), true);
    }
    void onLeft() noexcept
    {
        body->ApplyTorque(-torque, true);
    }
    void onRight() noexcept
    {
        body->ApplyTorque(torque, true);
    }
    void onAim() noexcept
    {
        sf::Vector2f targetRelativeToTurret;
        for (Turret& turret : turrets)
        {
            targetRelativeToTurret = getInverseTransform().transformPoint(aimCoords) - turret.getPosition();
            aimState = turret.setRotation(std::atan2(targetRelativeToTurret.y,  targetRelativeToTurret.x) * 180.f / pi);
        }
    }
    void onShoot() noexcept
    {
        if (clock.getElapsedTime().asSeconds() >= reload)
        {
            for (Turret& turret : turrets)
                turret.shoot(getTransform(), getRotation() / 180.f * pi, Vec2f::asVec2f(body->GetLinearVelocity()), -getId());
            clock.restart();
        }
    }
    std::wstring playerId;
    b2Body* body;
    b2MassData massData;
    float force, torque, reload, maxHp, hp, armor;
    bool aimState;
    std::vector<Turret> turrets;
    std::vector<Object::ObjectId> shields;
    sf::Clock clock;
    friend class Bot;
    friend class ContactListener;
    friend class RenderSerializer;
};

#endif
