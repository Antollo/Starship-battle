#ifndef SERVER_SHAPE_H
#define SERVER_SHAPE_H

#include <vector>
#include <SFML/Graphics/VertexBuffer.hpp>
#include "Object.h"
#include "Vec2f.h"
#include "Shape.h"

class ServerShape : public Shape
{
public:
    ServerShape(std::vector<Vec2f> newVertices, Vec2f newOrigin)
        : initialized(false), vertices(newVertices), origin(newOrigin) {}

    static IdType setShape(std::vector<Vec2f> vertices, Vec2f origin)
    {
        std::unordered_map<std::vector<Vec2f>, IdType>::iterator it;
        if ((it = shapeToId.find(vertices)) != shapeToId.end())
        {
            return it->second;
        }
        else
        {
            shapes.push_back({vertices, origin});
            shapeToId[vertices] = shapes.size();
            return shapes.size();
        }
    }

    static ServerShape *getShape(IdType id)
    {
        if (id != 0 && id <= shapes.size())
            return &shapes[id - 1];
        else
            return nullptr;
    }

    std::vector<Vec2f> vertices;
    Vec2f origin;

private:
    static inline std::vector<ServerShape> shapes;
    static inline std::unordered_map<std::vector<Vec2f>, IdType> shapeToId;
    bool initialized;
};

#endif /* SERVER_SHAPE_H */
