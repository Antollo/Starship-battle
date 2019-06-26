#ifndef GRID_H_
#define GRID_H_

#include <SFML/Graphics.hpp>

class Grid : public sf::VertexArray
{
public:
    Grid() : sf::VertexArray(sf::PrimitiveType::Lines, 402)
    {
        for (int i = -100; i <= 100; i += 4)
        {
            (*this)[i+100].color = sf::Color::White;
            (*this)[i+101].color = sf::Color::White;
            (*this)[i+102].color = sf::Color::White;
            (*this)[i+103].color = sf::Color::White;
            (*this)[i+100].position = {(float)i * 1000.f, -100000.f};
            (*this)[i+101].position = {(float)i * 1000.f, 100000.f};
            (*this)[i+102].position = {-100000.f, (float)i * 1000.f};
            (*this)[i+103].position = {100000.f, (float)i * 1000.f};
        }
    }
};

#endif /* !GRID_H_ */
