#include "prototypes.h"
#include "ServerShape.h"
#include "resourceManager.h"
#include "world.h"

void transformPoints(std::vector<Vec2f> &points)
{
    for (auto &point : points)
        point *= world::scale;
    points.push_back(points.front());
}

Vec2f getOrigin(b2PolygonShape &shape, std::vector<Vec2f> points)
{
    shape.Set(reinterpret_cast<b2Vec2 *>(points.data()), points.size());
    b2MassData massData;
    shape.ComputeMass(&massData, 1.f);
    return Vec2f::asVec2f(massData.center);
}

void from_json(const json &j, TurretPrototype &proto)
{
    proto.maxAngle = sf::degrees(j.at("maxAngle").get<float>());
    proto.bulletVelocity = j.at("bulletVelocity").get<float>();
    proto.accuracy = j.at("accuracy").get<float>();
    proto.origin = j.at("origin").get<Vec2f>() * world::scale;
    proto.points = j.at("points").get<std::vector<Vec2f>>();
    transformPoints(proto.points);
    proto.shapeId = ServerShape::setShape(proto.points, proto.origin);
    proto.rng.setParams(0, proto.accuracy / 1.3f);

    proto.bulletPrototype.points = j.at("bulletShape").get<std::vector<Vec2f>>();
    proto.bulletPrototype.origin = getOrigin(proto.bulletPrototype.shape, proto.bulletPrototype.points) * world::scale;
    transformPoints(proto.bulletPrototype.points);
    proto.bulletPrototype.shapeId = ServerShape::setShape(proto.bulletPrototype.points, proto.bulletPrototype.origin);

    proto.bulletPrototype.damage = j.at("damage").get<float>();
    proto.bulletPrototype.penetration = j.at("penetration").get<float>();
}

void from_json(const json &j, PositionedTurretPrototype &proto)
{
    proto.turretPrototype = &resourceManager::getTurretPrototype(j.at("type").get<std::string>());
    proto.position = Vec2f(j.at("x").get<float>(), j.at("y").get<float>());
}

void from_json(const json &j, ShieldPrototype &proto)
{
    proto.points = j.at("points").get<std::vector<Vec2f>>();
    std::sort(proto.points.begin(), proto.points.end(), [](const auto &a, const auto &b)
              { return std::atan2(a.y, a.x) < std::atan2(b.y, b.x); });
    proto.origin = getOrigin(proto.shape, proto.points) * world::scale;
    proto.position = Vec2f(j.at("x").get<float>(), j.at("y").get<float>());
    transformPoints(proto.points);
    proto.shapeId = ServerShape::setShape(proto.points, proto.origin);
}

void from_json(const json &j, SpaceshipPrototype &proto)
{
    proto.linearDamping = j.at("linearDamping").get<float>();
    proto.angularDamping = j.at("angularDamping").get<float>();
    proto.force = j.at("force").get<float>();
    proto.torque = j.at("torque").get<float>();
    proto.friction = j.at("friction").get<float>();
    proto.density = j.at("density").get<float>();
    proto.reload = j.at("reload").get<std::vector<float>>();
    proto.hp = j.at("hp").get<float>();
    proto.armor = j.at("armor").get<float>();

    proto.points = j.at("points").get<std::vector<Vec2f>>();
    proto.origin = getOrigin(proto.shape, proto.points) * world::scale;
    transformPoints(proto.points);
    proto.shapeId = ServerShape::setShape(proto.points, proto.origin);

    proto.turrets = j.at("turrets").get<std::vector<PositionedTurretPrototype>>();
    proto.shields = j.at("shields").get<std::vector<ShieldPrototype>>();
}