#ifndef SHIELD_H_
#define SHIELD_H_

#include <utility>
#include <stdexcept>
#include "Object.h"

class Shield : public Object
{
public:
    static Shield* create(std::vector<Vec2f> points, int index, b2Vec2 position)
    {
        return dynamic_cast<Shield*>(objects.emplace(counter, new Shield(points, index, position)).first->second.get());
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Shield;
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
    ~Shield() override
    {
        if(joint) world.DestroyJoint(joint);

        Rock::create(polygon, body);
    }
    void process(float delta) override
    {
        body->GetMassData(&massData);
        setOrigin(massData.center.x * worldScale, massData.center.y * worldScale);
        setPosition(body->GetPosition().x * worldScale, body->GetPosition().y * worldScale);
        setRotation(body->GetAngle() * 180.f / pi);
        setOrigin(0, 0);
        //joint->SetMotorSpeed(-100.f * (piPi(joint->GetJointAngle())));
        //std::cout<<piPi(joint->GetJointAngle())<<std::endl;
    }
    void connect(b2Body *shipBody, float x, float y)
    {
        b2WeldJointDef jointDef;
        jointDef.bodyA = shipBody;
        jointDef.bodyB = body;
        jointDef.localAnchorB = body->GetLocalCenter();
        jointDef.localAnchorB.x += x / 2.f;
        jointDef.localAnchorB.y += y / 2.f;

        jointDef.localAnchorA = shipBody->GetLocalCenter();
        jointDef.localAnchorA.x -= x / 2.f;
        jointDef.localAnchorA.y -= y / 2.f;

        jointDef.collideConnected = true;
        //jointDef.referenceAngle = 1;
        jointDef.frequencyHz = 8;
        jointDef.dampingRatio = 0.8;

        //jointDef.enableMotor = true;
        //jointDef.maxMotorTorque = 100;
        //jointDef
        //jointDef.motorSpeed = 100;
        joint = (b2WeldJoint*)world.CreateJoint(&jointDef);
    }
private:
    inline float piPi(float x){
        x = std::fmod(x + pi, 2 * pi);
        if (x < 0)
            x += 2 * pi;
        return x - pi;
    }
    Shield(std::vector<Vec2f>& points, int index, b2Vec2 position)
    {
        b2BodyDef bodyDef;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.01f;
        bodyDef.angularDamping = 1.f;
        bodyDef.userData = this;
        bodyDef.position = position;
        bodyDef.bullet = true;

        body = world.CreateBody(&bodyDef);

        std::sort(points.begin(), points.end(), [](const auto& a, const auto& b){
            return std::atan2(a.y, a.x) < std::atan2(b.y, b.x);
        });
        
        b2PolygonShape shape;
        shape.Set(reinterpret_cast<const b2Vec2*>(points.data()), points.size());

        b2FixtureDef fixtureDef;
        fixtureDef.density = 1.f;
        fixtureDef.friction = 0.1f;
        fixtureDef.filter.groupIndex = index; 
        fixtureDef.shape = &shape;
        body->CreateFixture(&fixtureDef);

        joint = nullptr;

        polygon.resize(points.size() + 1);
        polygon.setPrimitiveType(sf::PrimitiveType::LineStrip);
        for (std::size_t i = 0; i < points.size(); i++)
        {
            polygon[i].position = (sf::Vector2f) points[i] * worldScale;
            polygon[i].color = sf::Color::White;
        }
        polygon[points.size()] = polygon[0];
    }
    void draw(RenderSerializerBase& target, sf::RenderStates states, const Vec2f &position, const Vec2f &linearVelocity, float angularVelocity) const noexcept override
    {
        states.transform *= getTransform();
        target.draw(polygon, states, getCenterPosition(), getLinearVelocity(), getAngularVelocity());
    }
    friend class Spaceship;
    b2Body* body;
    b2WeldJoint* joint;
    b2MassData massData;
};

#endif
