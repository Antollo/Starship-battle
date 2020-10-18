#ifndef GRID_H_
#define GRID_H_

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

class Grid : public sf::Drawable
{
public:
    Grid() : vertexArray(sf::PrimitiveType::Lines, 402)
    {
        for (int i = -100; i <= 100; i += 4)
        {
            vertexArray[i + 100].color = semiTransparentWhite;
            vertexArray[i + 101].color = semiTransparentWhite;
            vertexArray[i + 102].color = semiTransparentWhite;
            vertexArray[i + 103].color = semiTransparentWhite;
            vertexArray[i + 100].position = {(float)i * 1000.f, -100000.f};
            vertexArray[i + 101].position = {(float)i * 1000.f, 100000.f};
            vertexArray[i + 102].position = {-100000.f, (float)i * 1000.f};
            vertexArray[i + 103].position = {100000.f, (float)i * 1000.f};
        }
    }
private:
    virtual void draw(sf::RenderTarget &target, sf::RenderStates _) const
    {
        target.draw(vertexArray);
    }
    sf::VertexArray vertexArray;
    static inline const sf::Color semiTransparentWhite = sf::Color(255, 255, 255, 160);
};

#endif /* !GRID_H_ */
