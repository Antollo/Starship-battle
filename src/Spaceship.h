#ifndef SPACESHIP_H_
#define SPACESHIP_H_

#include <fstream>
#include <utility>
#include <stdexcept>
#include "Object.h"
#include "Turret.h"
#include "Rock.h"

class Spaceship : public Object, public sf::Transformable
{
public:
    static Spaceship* create(const std::string& type, const std::wstring& playerId)
    {
        if (type.empty()) throw std::runtime_error("Type of spaceship was empty.");
        if (playerId.empty()) throw std::runtime_error("Commander ID was empty.");
        return dynamic_cast<Spaceship*>(objects.emplace(counter, new Spaceship(type, playerId)).first->second.get());
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
    float getReloadState()
    {
        return clock.getElapsedTime().asSeconds() / reload;
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
        Object::process();
    }
    bool forward, left, right, aim, shoot;
    Vec2f aimCoords;
private:
    Spaceship(const std::string& type, const std::wstring& newPlayerId = L"AutomatedPilot-" + std::to_wstring(counter + 1)) : forward(false), left(false), right(false), aim(false), shoot(false), playerId(newPlayerId)
    {
        std::ifstream file(type + ".json");
        if (!file.good()) throw std::runtime_error("Informations about spaceship not found.");
        json jsonObject = json::parse(file);

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
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states);
        for (const Turret& turret : turrets)
            target.draw(turret, states);
    }
    void draw(RenderSerializerBase& target, sf::RenderStates states) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states);
        for (const Turret& turret : turrets)
            target.draw(turret, states);
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
            turret.setRotation(std::atan2(targetRelativeToTurret.y,  targetRelativeToTurret.x) * 180.f / pi);
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
    sf::VertexArray polygon;
    b2Body* body;
    b2MassData massData;
    float force, torque, reload, maxHp, hp, armor;
    std::vector<Turret> turrets;
    sf::Clock clock;
    friend class Bot;
    friend class Cursor;
    friend class ContactListener;
    friend class RenderSerializer;
};

#endif
