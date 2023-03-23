#ifndef CLIENT_SHAPE_H
#define CLIENT_SHAPE_H

#include <vector>
#include <SFML/Graphics/VertexBuffer.hpp>
#include "Object.h"
#include "Vec2f.h"
#include "Shape.h"

class ClientShape : public Shape
{
public:
    ClientShape()
        : initialized(false), body(sf::PrimitiveType::TriangleFan), outline(sf::PrimitiveType::LineStrip)
    {
    }

    void load(const std::vector<sf::Vector2f> &vertices, sf::Vector2f newOrigin)
    {
        // TODO: shader with const colors
        std::vector<sf::Vertex> buffer(vertices.size());
        for (size_t i = 0; i < vertices.size(); i++)
        {
            buffer[i].color = semiTransparentBlack;
            buffer[i].position = vertices[i];
        }
        body.create(buffer.size());
        body.update(&buffer[0]);
        for (size_t i = 0; i < vertices.size(); i++)
        {
            buffer[i].color = sf::Color::White;
        }
        outline.create(buffer.size());
        outline.update(&buffer[0]);

        origin = newOrigin;

        initialized = true;
    }

    sf::VertexBuffer body, outline;
    sf::Vector2f origin;

    static void setShape(IdType id, const std::vector<sf::Vector2f> &vertices, sf::Vector2f origin)
    {
        if(id >= shapes.size())
            shapes.resize(id + 1);
        shapes[id].load(vertices, origin);
    }

    static ClientShape &getShape(IdType id)
    {
        if (id != 0 && id < shapes.size() && shapes[id].initialized)
            return shapes[id];
        else
        {
            static ClientShape shape;
            if (!shape.initialized)
            {
                std::vector<sf::Vector2f> vertices(4);
                vertices[0] = sf::Vector2f(0, 0) * 40.f;
                vertices[1] = sf::Vector2f(3, 1) * 40.f;
                vertices[2] = sf::Vector2f(0, 2) * 40.f;
                vertices[3] = sf::Vector2f(0, 0) * 40.f;
                shape.load(vertices, {40.f, 40.f});
            }
            return shape;
        }
    }

    static bool hasShape(IdType id)
    {
        return id == 0 || id < shapes.size() && shapes[id].initialized;
    }

private:
    static inline const sf::Color semiTransparentBlack = sf::Color(0, 0, 0, 200);
    static inline std::vector<ClientShape> shapes;
    bool initialized;
};

#endif /* CLIENT_SHAPE_H */
