#ifndef BULLET_H_
#define BULLET_H_

#include "Object.h"

class Spaceship;

class Bullet : public Object
{
public:
    static Bullet* create(const std::vector<Vec2f>& points, Vec2f position, const float& angle, const float& newPenetration, const float& newDamage, const Vec2f& velocity, const int& index)
    {
        //objects.emplace_back(new Bullet(points, position, angle, penetrationAngle, velocity, index));
        //return dynamic_cast<Bullet*>(objects.back().get());
        return dynamic_cast<Bullet*>(objects.emplace(counter, new Bullet(points, position, angle, newPenetration, newDamage, velocity, index)).first->second.get());
    }
    const Object::TypeId getTypeId() const override
    {
        return Object::TypeId::Bullet;
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
    ~Bullet() override
    {
        world.DestroyBody(body);
    }
    void process(float delta) override
    {
        body->GetMassData(&massData);
        setOrigin(massData.center.x * worldScale, massData.center.y * worldScale);
        setPosition(body->GetPosition().x * worldScale, body->GetPosition().y * worldScale);
        setRotation(body->GetAngle() * 180.f / pi);
        setOrigin(0, 0);
		destroy = destroy || body->GetLinearVelocity().LengthSquared() < minimumBulletVelocity;
    }
    static constexpr float minimumBulletVelocity = 1.f;
private:
    Bullet(const std::vector<Vec2f>& points, const Vec2f& position, const float& angle, const float& newPenetration, const float& newDamage, const Vec2f& velocity, const int& index)
        : penetration(newPenetration), damage(newDamage)
    {
        b2BodyDef bodyDef;
        bodyDef.position = position;
        bodyDef.angle = angle;
        bodyDef.type = b2_dynamicBody;
        bodyDef.linearDamping = 0.1f;
        bodyDef.angularDamping = 0.1f;
        bodyDef.bullet = true;
        bodyDef.userData = this;

        body = world.CreateBody(&bodyDef);

        b2PolygonShape shape;
        shape.Set(reinterpret_cast<const b2Vec2*>(points.data()), points.size());

        b2FixtureDef fixtureDef;
        fixtureDef.density = 7.5f;
        fixtureDef.friction = 0.2f;
        fixtureDef.filter.groupIndex = index;
        fixtureDef.shape = &shape;
        body->CreateFixture(&fixtureDef);
        body->GetMassData(&massData);

        std::vector<Vec2f> pointsCopy(points);

        pointsCopy.resize(pointsCopy.size() + 1);
        for (size_t i = 0; i < pointsCopy.size(); i++)
            pointsCopy[i] = pointsCopy[i] * worldScale;
        pointsCopy.back() = pointsCopy.front();

        RenderSerializable::shape = ServerShape::setShape(pointsCopy, {massData.center.x * worldScale, massData.center.y * worldScale});

        body->SetLinearVelocity(velocity);
        process(0);
    }
    ObjectId getId() override { return -body->GetFixtureList()[0].GetFilterData().groupIndex; }
    b2Body* body;
    b2MassData massData;
    float penetration, damage;
    Spaceship* lastContact = nullptr;
    friend class ContactListener;
};

#endif
