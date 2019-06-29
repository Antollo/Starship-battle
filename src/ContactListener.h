#ifndef CONTACTLISTENER_H_
#define CONTACTLISTENER_H_

#include "Spaceship.h"
#include "Bot.h"
#include "Bullet.h"
#include <queue>
#include "Event.h"


class ContactListener : public b2ContactListener
{
public:
    ContactListener(std::queue<DownEvent>& newDownEvents)
        : downEvents(newDownEvents) {}
private:
    void BeginContact(b2Contact* contact)
    {
        Spaceship* spaceship = nullptr;
        Bullet* bullet = nullptr;

        void* bodyUserData = contact->GetFixtureA()->GetBody()->GetUserData();
        if (bodyUserData != nullptr)
        {
            if (bodyUserData)
            {
                switch(static_cast<Object*>(bodyUserData)->getTypeId())
                {
                case Object::TypeId::Bullet:
                    bullet = static_cast<Bullet*>(bodyUserData);
                    break;
                case Object::TypeId::Spaceship: //Intended behaviour (both are spaceships)
                case Object::TypeId::Bot:
                    spaceship = static_cast<Spaceship*>(bodyUserData);
                    break;
                case Object::TypeId::Rock:
                    //resourceManager::playSound("stone.wav");
                default:
                    break;
                }
            }
        }

        bodyUserData = contact->GetFixtureB()->GetBody()->GetUserData();
        if (bodyUserData != nullptr)
        {
            if (bodyUserData)
            {
                switch(static_cast<Object*>(bodyUserData)->getTypeId())
                {
                case Object::TypeId::Bullet:
                    bullet = static_cast<Bullet*>(bodyUserData);
                    break;
                case Object::TypeId::Spaceship: //Intended behaviour (both are spaceships)
                case Object::TypeId::Bot:
                    spaceship = static_cast<Spaceship*>(bodyUserData);
                    break;
                case Object::TypeId::Rock:
                    //resourceManager::playSound("stone.wav");
                default:
                    break;
                }
            }
        }


        if (spaceship != nullptr && bullet != nullptr)
        {
            b2WorldManifold worldManifold;
            contact->GetWorldManifold(&worldManifold);
            b2Vec2 vel1 = bullet->body->GetLinearVelocityFromWorldPoint(worldManifold.points[0]);
            b2Vec2 vel2 = spaceship->body->GetLinearVelocityFromWorldPoint(worldManifold.points[0]);
            Vec2f impactVelocity = vel1 - vel2;

            std::vector<std::pair<Vec2f, Vec2f>> edges = spaceship->getEdges();
            Vec2f collisionPos = worldManifold.points[0];

            //particleSystem.impulse((sf::Vector2f)Vec2f::asVec2f(worldManifold.points[0]) * Object::worldScale);


            auto it = std::min_element(edges.begin(), edges.end(), [&collisionPos](const std::pair<Vec2f, Vec2f>& a, const std::pair<Vec2f, Vec2f>& b) {
                return a.first.getSquaredDistance(collisionPos) + a.second.getSquaredDistance(collisionPos) < b.first.getSquaredDistance(collisionPos) + b.second.getSquaredDistance(collisionPos);
            });

            Vec2f edge {it->first.x - it->second.x, it->first.y - it->second.y};
            float angle = std::atan2(impactVelocity.x*edge.y - impactVelocity.y*edge.x, impactVelocity.x*edge.x + impactVelocity.y*edge.y);
            if (angle < 0.f) angle += 2.f *  pi;
            //float damage = std::round(impactVelocity.getSquaredLength() * bullet->massData.mass / 2.f * Object::rng025(Object::mt));
            float damage = std::roundf(bullet->damage * Object::rng025(Object::mt));
            float penetration = bullet->penetration * Object::rng025(Object::mt);
            if (std::min(angle, pi - angle) > pi / 6.f && impactVelocity.getLength() > 18.f && penetration > spaceship->armor)
            {
                bullet->destroy = true;
                spaceship->hp -= damage;
                //console << bullet->getId()
                //<< L" hitted " << spaceship->getId()
                //<< L" for " << damage
                //<< L"\n";
                //resourceManager::playSound("explosion.wav");

                downEvents.emplace(DownEvent::Type::Collision);
                downEvents.back().collision = (sf::Vector2f)Vec2f::asVec2f(worldManifold.points[0]) * Object::worldScale;
                downEvents.back().explosion = true;
                downEvents.back().message = dynamic_cast<Spaceship&>(*Object::objects[bullet->getId()]).playerId +
                    L" hitted " + spaceship->playerId + L" for " + std::to_wstring((int)damage) + L"\n";
                if (spaceship->hp <= 0.f)
                {
                    downEvents.emplace(DownEvent::Type::Message);
                    downEvents.back().message = spaceship->playerId + L" was warped to HQ\n";
                }

                if (spaceship->getTypeId() == Object::TypeId::Bot)
                {
                    dynamic_cast<Bot*>(spaceship)->target(bullet->getId());
                }
            }
            else
            {
                //console << spaceship->getId()
                //<< L" bounced the bullet\n";
                //resourceManager::playSound("ricochet.ogg");

                downEvents.emplace(DownEvent::Type::Collision);
                downEvents.back().collision = (sf::Vector2f)Vec2f::asVec2f(worldManifold.points[0]) * Object::worldScale;
                downEvents.back().explosion = false;
                downEvents.back().message = spaceship->playerId + L" bounced the bullet\n";
            }
            //downEvents.emplace(DownEvent::Type::Message);
            //downEvents.back().message = std::to_wstring((int)penetration) + L" " + std::to_wstring((int)spaceship->armor) + L" " + std::to_wstring((int)damage) + L" " + std::to_wstring((int)impactVelocity.getLength()) + L"\n";
        }
  
    }
  
    void EndContact(b2Contact* contact)
    { }
    std::queue<DownEvent>& downEvents;
};

#endif
