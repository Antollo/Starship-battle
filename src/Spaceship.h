#ifndef SPACESHIP_H_
#define SPACESHIP_H_

#include <fstream>
#include <utility>
#include <stdexcept>
#include "Object.h"
#include "Turret.h"
#include "Console.h"

class Spaceship : public Object, public sf::Transformable
{
public:
    static Spaceship* create(const std::string& type, const std::wstring& playerID)
    {
        if (type.empty()) throw std::runtime_error("Type of spaceship was empty.");
        if (playerID.empty()) throw std::runtime_error("Commander ID was empty.");
        objects.emplace_back(new Spaceship(type, playerID));
        return dynamic_cast<Spaceship*>(objects.back().get());
    }
    const Object::typeId getTypeId() const noexcept override
    {
        return Object::typeId::Spaceship;
    }
    sf::Vector2f getCenterPosition() noexcept
    {
        body->GetMassData(&massData);
        return getTransform().transformPoint(massData.center.x * worldScale, massData.center.y * worldScale);
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
        (*Object::console) << Spaceship::playerIDs[getID()]
        << L" was warped to HQ\n";
        world.DestroyBody(body);
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
    Spaceship(const std::string& type, const std::wstring& playerID) : forward(false), left(false), right(false), aim(false), shoot(false)
    {
        static_assert(sizeof(b2Vec2) == sizeof(Vec2f), "error");
        static_assert(sizeof(sf::Vector2f) == sizeof(Vec2f), "error");

        std::ifstream file(type + ".txt");
        if (!file.good()) throw std::runtime_error("Informations about spaceship not found.");

        counter++;
        playerIDs.push_back(playerID);

        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        file >> bodyDef.linearDamping;
        file >> bodyDef.angularDamping;
        bodyDef.userData = this;
        bodyDef.position.x = (rng01(mt) * worldLimits * 2.f - worldLimits)/worldScale;
        bodyDef.position.y = (rng01(mt) * worldLimits * 2.f - worldLimits)/worldScale;

        body = world.CreateBody(&bodyDef);

        std::size_t n;
        file >> n;
        std::vector<Vec2f> points(n);
        for (auto& point : points)
            file >> point.x >> point.y;

        b2PolygonShape shape;
        shape.Set(reinterpret_cast<b2Vec2*>(points.data()), n);

        b2FixtureDef fixtureDef;
        file >> fixtureDef.density;
        file >> fixtureDef.friction;
        fixtureDef.filter.groupIndex = -counter; 
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

        file >> force >> torque >> reload >> hp;

        std::string str;
        float x, y;
        file >> n;
        while (n--)
        {
            file >> str >> x >> y;
            turrets.emplace_back(str);
            turrets.back().setPosition(x * worldScale, y * worldScale);
        }
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept override
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
            turret.setRotation(std::atan2f(targetRelativeToTurret.y,  targetRelativeToTurret.x) * 180.f / pi);
        }
    }
    void onShoot() noexcept
    {
        if (clock.getElapsedTime().asSeconds() >= reload)
        {
            for (Turret& turret : turrets)
                turret.shoot(getTransform(), getRotation() / 180.f * pi, Vec2f::asVec2f(body->GetLinearVelocity()), -getID());
            clock.restart();
        }
    }
    inline int getID()
    {
        return -body->GetFixtureList()[0].GetFilterData().groupIndex;
    }
    sf::VertexArray polygon;
    b2Body* body;
    b2MassData massData;
    float force, torque, reload, hp;
    std::vector<Turret> turrets;
    sf::Clock clock;
    static std::vector<std::wstring> playerIDs;
    friend class Bot;
    friend class Cursor;
    friend class ContactListener;
};

#endif
