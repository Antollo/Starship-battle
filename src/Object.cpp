#include "Object.h"
#include "Map.h"

Object::ObjectId Object::counter = 1;
Object::ObjectId Object::thisPlayerId = -1;
Object::ObjectsContainer Object::objects;
b2World Object::world = b2Vec2(0, 0);
std::random_device Object::rd;
std::mt19937 Object::mt(rd());
Map *Object::map = nullptr;

void Object::setMap(Map *m)
{
    if (map != nullptr)
        delete map;
    map = m;
}