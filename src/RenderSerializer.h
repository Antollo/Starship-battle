#ifndef RENDERSERIALIZER_H_
#define RENDERSERIALIZER_H_

#include <SFML/Graphics.hpp>
#include "Event.h"
#include "Object.h"
#include "Spaceship.h"

class RenderSerializer : public RenderSerializerBase
{
public:
    RenderSerializer() : downEvent(DownEvent::Type::DirectDraw) {}
    void draw(const RenderSerializable& drawable, const sf::RenderStates& states = sf::RenderStates::Default) noexcept override
    {
        drawable.draw(*this, states);        
    }
    void draw(const sf::VertexArray& vertexArray, const sf::RenderStates& states = sf::RenderStates::Default) noexcept override
    {
        downEvent.polygons.push_back({vertexArray, states});
    }
    void clear()
    {
        downEvent.polygons.clear();
        downEvent.players.clear();
    }
    DownEvent getDownEvent()
    {
        for (const auto& it : Object::objects)
        {
            if (it.second->getTypeId() == Object::TypeId::Spaceship || it.second->getTypeId() == Object::TypeId::Bot)
            {
                downEvent.players.push_back({
                    it.first,
                    dynamic_cast<Spaceship&>(*it.second).playerId,
                    it.second->getCenterPosition(),
                    dynamic_cast<Spaceship&>(*it.second).getReloadState(),
                    static_cast<std::int16_t>(dynamic_cast<Spaceship&>(*it.second).hp),
                    static_cast<std::int16_t>(dynamic_cast<Spaceship&>(*it.second).maxHp)
                });
            }
        }
        return downEvent;
    }
private:
    DownEvent downEvent;
};

#endif /* !RENDERSERIALIZER_H_ */
