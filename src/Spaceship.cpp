#include "Spaceship.h"
#include "Map.h"

Spaceship::Spaceship(const SpaceshipPrototype &prototype, const std::wstring &newPlayerId, ObjectId objectId)
    : proto(prototype), Object(objectId), forward(false), left(false), right(false), shoot(false), playerId(newPlayerId)
{
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.linearDamping = proto.linearDamping;
    bodyDef.angularDamping = proto.angularDamping;
    bodyDef.userData = this;
    bodyDef.position = Map::getMap().randomPosition();
    bodyDef.angle = Rng::uniform() * 2.f * pi;

    body = world.CreateBody(&bodyDef);

    b2FixtureDef fixtureDef;
    fixtureDef.density = proto.density;
    fixtureDef.friction = proto.friction;
    fixtureDef.filter.groupIndex = -getId();
    fixtureDef.shape = &proto.shape;
    fixtureDef.thickShape = true;
    body->CreateFixture(&fixtureDef);

    reloadIt = proto.reload.begin();
    hp = proto.hp;

    for (auto &turret : proto.turrets)
    {
        turrets.emplace_back(*turret.turretPrototype);
        turrets.back().setPosition(turret.position * world::scale);
    }

    for (const auto &shield : proto.shields)
    {
        auto shieldPtr = Shield::create(shield, -getId(), bodyDef.position);
        shieldPtr->connect(body, shield.position.x, shield.position.y);
        shields.push_back(shieldPtr->getId());
    }
}