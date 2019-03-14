#ifndef CONTACTLISTENER_H_
#define CONTACTLISTENER_H_

#include "Spaceship.h"
#include "Bot.h"
#include "Console.h"
#include "Bullet.h"
#include "ParticleSystem.h"
#include "resourceManager.h"


class ContactListener : public b2ContactListener
{
    void BeginContact(b2Contact* contact)
    {
        Spaceship* spaceship = nullptr;
        Bullet* bullet = nullptr;

        void* bodyUserData = contact->GetFixtureA()->GetBody()->GetUserData();
        if (bodyUserData)
        {
            switch(static_cast<Object*>(bodyUserData)->getTypeId())
            {
            case Object::typeId::Bullet:
                bullet = static_cast<Bullet*>(bodyUserData);
                break;
            case Object::typeId::Spaceship:
            case Object::typeId::Bot:
                spaceship = static_cast<Spaceship*>(bodyUserData);
                break;
            case Object::typeId::Rock:
                //resourceManager::playSound("stone.wav");
            default:
                break;
            }
        }
        bodyUserData = contact->GetFixtureB()->GetBody()->GetUserData();
        if (bodyUserData)
        {
            switch(static_cast<Object*>(bodyUserData)->getTypeId())
            {
            case Object::typeId::Bullet:
                bullet = static_cast<Bullet*>(bodyUserData);
                break;
            case Object::typeId::Spaceship:
            case Object::typeId::Bot:
                spaceship = static_cast<Spaceship*>(bodyUserData);
                break;
            case Object::typeId::Rock:
                //resourceManager::playSound("stone.wav");
            default:
                break;
            }
        }
        if (spaceship != nullptr && bullet != nullptr)
        {
            b2WorldManifold worldManifold;
            contact->GetWorldManifold(&worldManifold);
            b2Vec2 vel1 = bullet->body->GetLinearVelocityFromWorldPoint(worldManifold.points[0]);
            b2Vec2 vel2 = spaceship->body->GetLinearVelocityFromWorldPoint(worldManifold.points[0]);
            b2Vec2 impactVelocity = vel1 - vel2;

            std::vector<std::pair<Vec2f, Vec2f>> edges = spaceship->getEdges();
            Vec2f collisionPos = Vec2f::asVec2f(worldManifold.points[0]);

            Object::particleSystem->impulse((sf::Vector2f)Vec2f::asVec2f(worldManifold.points[0]) * Object::worldScale);

            auto it = std::min_element(edges.begin(), edges.end(), [&collisionPos](const std::pair<Vec2f, Vec2f>& a, const std::pair<Vec2f, Vec2f>& b) {
                return a.first.squaredDistance(collisionPos) + a.second.squaredDistance(collisionPos) < b.first.squaredDistance(collisionPos) + b.second.squaredDistance(collisionPos);
            });

            Vec2f edge {it->first.x - it->second.x, it->first.y - it->second.y};
            float angle = atan2(impactVelocity.x*edge.y - impactVelocity.y*edge.x, impactVelocity.x*edge.x + impactVelocity.y*edge.y);
            if (angle < 0.f) angle += 2.f *  pi;
            if (std::min(angle, pi - angle) > bullet->minAngle * Object::rng025(Object::mt))
            {
                float damage = (float) bullet->massData.mass * Object::rng025(Object::mt) * 100.f;
                bullet->destroy = true;
                spaceship->hp -= damage;
                (*Object::console) << Spaceship::playerIDs[bullet->getID()]
                << L" hitted " << Spaceship::playerIDs[spaceship->getID()] 
                << L" for " << damage
                << L"\n";
                resourceManager::playSound("explosion.wav");
            }
            else
            {
                (*Object::console) << Spaceship::playerIDs[spaceship->getID()]
                << L" bounced the bullet\n";
                resourceManager::playSound("ricochet.ogg");
            }
        }
  
    }
  
    void EndContact(b2Contact* contact)
    {
        //std::cout<<"end\n";
        //void* bodyUserData = contact->GetFixtureA()->GetBody()->GetUserData();
        //if (bodyUserData)
        //    std::cout<<"end\n";
    }
};

#endif
