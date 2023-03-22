#include "Spaceship.h"
#include "Map.h"

Spaceship::Spaceship(const std::string &type, const std::wstring &newPlayerId, ObjectId objectId) : Object(objectId), forward(false), left(false), right(false), shoot(false), playerId(newPlayerId)
{
    json jsonObject = resourceManager::getJSON(type);

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.linearDamping = jsonObject["linearDamping"].get<float>();
    bodyDef.angularDamping = jsonObject["angularDamping"].get<float>();
    bodyDef.userData = this;
    bodyDef.position = Map::getMap().randomPosition();
    //bodyDef.position.x = (uniformRNG<0, 1, 1, 1>() * worldLimits * 2.f - worldLimits)/worldScale;
    //bodyDef.position.y = (uniformRNG<0, 1, 1, 1>() * worldLimits * 2.f - worldLimits)/worldScale;
    bodyDef.angle = Rng::uniform<0, 1, 1, 1>() * 2.f * pi;

    body = world.CreateBody(&bodyDef);

    std::vector<Vec2f> points = jsonObject["points"].get<std::vector<Vec2f>>(); // (n);
    std::size_t n = points.size();

    b2PolygonShape shape;
    shape.Set(reinterpret_cast<b2Vec2 *>(points.data()), n);

    b2FixtureDef fixtureDef;
    fixtureDef.density = jsonObject["density"].get<float>();
    fixtureDef.friction = jsonObject["friction"].get<float>();
    fixtureDef.filter.groupIndex = -getId();
    fixtureDef.shape = &shape;
    fixtureDef.thickShape = true;
    body->CreateFixture(&fixtureDef);

    body->GetMassData(&massData);

    points.resize(n + 1);
    for (size_t i = 0; i < n; i++)
        points[i] = points[i] * worldScale;
    points[n] = points[0];

    RenderSerializable::shape = ServerShape::setShape(points, {massData.center.x * worldScale, massData.center.y * worldScale});

    force = jsonObject["force"].get<float>();
    torque = jsonObject["torque"].get<float>();
    reload = jsonObject["reload"].get<std::vector<float>>();
    reloadIt = reload.begin();
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
    for (auto &turret : jsonObject["turrets"])
    {
        turrets.emplace_back(turret["type"].get<std::string>());
        turrets.back().setPosition(turret["x"].get<float>() * worldScale, turret["y"].get<float>() * worldScale);
    }

    Shield *shield;
    const auto &shieldsJson = jsonObject["shields"];

    for (const auto &shieldJson : shieldsJson)
    {
        shield = Shield::create(shieldJson["points"].get<std::vector<Vec2f>>(), -getId(), bodyDef.position);
        shield->connect(body, shieldJson["x"].get<float>(), shieldJson["y"].get<float>());
        shields.push_back(shield->getId());
    }
}