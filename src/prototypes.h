#ifndef PROTOTYPES_H
#define PROTOTYPES_H

#include <vector>
#include <SFML/System/Angle.hpp>
#include <Box2D.h>
#include "Vec2f.h"
#include "Rng.h"
#include "Shape.h"

class Prototype
{
public:
    std::vector<Vec2f> points;
};

class BulletPrototype
{
public:
    b2PolygonShape shape;
    std::vector<Vec2f> points;
    Vec2f origin;
    Shape::IdType shapeId;
    float damage;
    float penetration;
};

class TurretPrototype
{
public:
    BulletPrototype bulletPrototype;
    std::vector<Vec2f> points;
    Vec2f origin;
    sf::Angle maxAngle;
    float bulletVelocity;
    float accuracy;
    Shape::IdType shapeId;
    mutable Rng::Normal rng;
};

class PositionedTurretPrototype
{
public:
    const TurretPrototype *turretPrototype;
    Vec2f position;
};

class ShieldPrototype
{
public:
    b2PolygonShape shape;
    std::vector<Vec2f> points;
    Vec2f origin;
    Vec2f position;
    Shape::IdType shapeId;
};

class SpaceshipPrototype
{
public:
    float linearDamping;
    float angularDamping;
    float force;
    float torque;
    float friction;
    float density;
    std::vector<float> reload;
    float hp;
    float armor;
    std::vector<Vec2f> points;
    b2PolygonShape shape;
    Shape::IdType shapeId;
    Vec2f origin;
    std::vector<PositionedTurretPrototype> turrets;
    std::vector<ShieldPrototype> shields;
};

void from_json(const json &j, TurretPrototype &proto);
void from_json(const json &j, PositionedTurretPrototype &proto);
void from_json(const json &j, ShieldPrototype &proto);
void from_json(const json &j, SpaceshipPrototype &proto);

#endif /* PROTOTYPES_H */
