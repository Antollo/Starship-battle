#include "Object.h"
#include "Map.h"

Object::ObjectId Object::counter = 1;
Object::ObjectId Object::thisPlayerId = -1;
Object::ObjectsContainer Object::objects;
b2World Object::world = b2Vec2(0, 0);