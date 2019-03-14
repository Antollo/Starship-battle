#include "Object.h"

int Object::counter = 0;
Console* Object::console = nullptr;
Spaceship* Object::spaceship = nullptr;
ParticleSystem* Object::particleSystem = nullptr;
Object::ObjectsContainer Object::objects;
b2World Object::world = b2Vec2(0, 0);
std::random_device Object::rd;
std::mt19937 Object::mt(rd());
std::uniform_real_distribution<float> Object::rng025(0.75f, 1.25f), Object::rng01(0.f, 1.f);