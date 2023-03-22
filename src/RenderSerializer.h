#ifndef RENDERSERIALIZER_H_
#define RENDERSERIALIZER_H_

#include <SFML/Graphics.hpp>
#include "Event.h"
#include "Object.h"
#include "Spaceship.h"

class RenderSerializer : public RenderSerializerBase
{
public:
    RenderSerializer() : downEvent(DownEvent::Type::DirectDraw), polygonsIndex(0), playersIndex(0) {}
    void draw(const RenderSerializable &drawable) noexcept override
    {
        drawable.draw(*this);
    }
    void draw(Shape::IdType shape, float rotation, const Vec2f &position, const Vec2f &linearVelocity, float angularVelocity) noexcept override
    {
        if (polygonsIndex < downEvent.polygons.size())
            downEvent.polygons[polygonsIndex] = {shape, rotation, position, linearVelocity, angularVelocity};
        else
            downEvent.polygons.push_back({shape, rotation, position, linearVelocity, angularVelocity});
        polygonsIndex++;
    }
    void clear()
    {
        //downEvent.polygons.clear();
        //downEvent.players.clear();
        polygonsIndex = 0;
        playersIndex = 0;
    }
    const DownEvent& getDownEvent()
    {
        for (const auto &it : Object::objects)
        {
            if (it.second->getTypeId() == Object::TypeId::Spaceship || it.second->getTypeId() == Object::TypeId::Bot)
            {
                if (playersIndex < downEvent.players.size())
                    downEvent.players[playersIndex] = {it.first,
                                                       dynamic_cast<Spaceship &>(*it.second).playerId,
                                                       it.second->getCenterPosition(),
                                                       it.second->getLinearVelocity(),
                                                       dynamic_cast<Spaceship &>(*it.second).getReloadState(),
                                                       dynamic_cast<Spaceship &>(*it.second).getAimState(),
                                                       static_cast<std::int16_t>(dynamic_cast<Spaceship &>(*it.second).hp),
                                                       static_cast<std::int16_t>(dynamic_cast<Spaceship &>(*it.second).maxHp)};
                else
                    downEvent.players.push_back({it.first,
                                                 dynamic_cast<Spaceship &>(*it.second).playerId,
                                                 it.second->getCenterPosition(),
                                                 it.second->getLinearVelocity(),
                                                 dynamic_cast<Spaceship &>(*it.second).getReloadState(),
                                                 dynamic_cast<Spaceship &>(*it.second).getAimState(),
                                                 static_cast<std::int16_t>(dynamic_cast<Spaceship &>(*it.second).hp),
                                                 static_cast<std::int16_t>(dynamic_cast<Spaceship &>(*it.second).maxHp)});
                playersIndex++;
            }
        }
        downEvent.polygons.resize(polygonsIndex);
        downEvent.players.resize(playersIndex);
        return downEvent;
    }

private:
    DownEvent downEvent;
    int polygonsIndex, playersIndex;
};

#endif /* !RENDERSERIALIZER_H_ */
