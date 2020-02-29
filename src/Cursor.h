#ifndef CURSOR_H_
#define CURSOR_H_

#include <sstream>
#include "Object.h"
#include "Spaceship.h"
#include "resourceManager.h"

std::wstring to_wstring_with_precision(const float &value)
{
    std::wstringstream out;
    out.precision(0);
    out << std::fixed << value;
    return out.str();
}

class Cursor : public sf::Drawable, public sf::Transformable
{
public:
    Cursor() : reloadState(0.f), aimState(true)
    {
        text.setFont(resourceManager::getFont("UbuntuMono.ttf"));
        text.setCharacterSize(18u);
        text.setFillColor(sf::Color::White);
        text.setOrigin(-64.f, (float)text.getCharacterSize());

        polygon.resize(4);
        polygon.setPrimitiveType(sf::PrimitiveType::Lines);
        polygon[0].position = {-8, 26};
        polygon[1].position = {0, 0};
        polygon[2].position = {0, 0};
        polygon[3].position = {8, 26};
        polygon[0].color = sf::Color::White;
        polygon[1].color = sf::Color::White;
        polygon[2].color = sf::Color::White;
        polygon[3].color = sf::Color::White;

        dots.resize(32);
        dots.setPrimitiveType(sf::PrimitiveType::Points);
        for (size_t i = 0; i < 32; i++)
        {
            dots[i].position = {cosf(float(i) * pi / 16.f) * 42.f, sinf(float(i) * pi / 16.f) * 42.f};
            dots[i].color = sf::Color::White;
        }

        circle.setRadius(56.f);
        circle.setOrigin(56.f, 56.f);
        circle.setOutlineColor(sf::Color::White);
        circle.setOutlineThickness(resourceManager::getJSON("config")["lineWidth"].get<float>());
        circle.setFillColor(sf::Color::Transparent);
    }
    void setState(const float &newReloadState = 0.f, bool newAimState = true, const int &hp = 0, const int &maxHp = 0)
    {
        reloadState = newReloadState;
        if (aimState != newAimState)
        {
            if (newAimState)
            {
                polygon[0].position = {-8, 26};
                polygon[1].position = {0, 0};
                polygon[2].position = {0, 0};
                polygon[3].position = {8, 26};
            }
            else
            {
                polygon[0].position = {-4, 26};
                polygon[1].position = {4, 0};
                polygon[2].position = {-4, 0};
                polygon[3].position = {4, 26};
            }
            aimState = newAimState;
        }
        if (hp)
            hpState = L"Hp: "s + std::to_wstring(hp) + L"/"s + std::to_wstring(maxHp);
        else
            hpState = L""s;
    }

private:
    void draw(sf::RenderTarget &target, sf::RenderStates states) const noexcept override
    {
        Cursor &hack = const_cast<Cursor &>(*this);
        hack.setScale(target.getView().getSize().x / (float)target.getSize().x, target.getView().getSize().y / (float)target.getSize().y);
        sf::Vector2f mousePos = target.mapPixelToCoords(sf::Mouse::getPosition(*dynamic_cast<sf::RenderWindow *>(&target)));
        hack.setPosition(mousePos);
        hack.text.setString(L"X: "s + to_wstring_with_precision(std::roundf(mousePos.x)) + L"\nY: "s + to_wstring_with_precision(std::roundf(mousePos.y)));
        states.transform = getTransform();
        target.draw(text, states);
        target.draw(polygon, states);
        target.draw(circle, states);

        size_t reloaded = (size_t)std::min(reloadState * 32.f, 32.f);
        for (size_t i = 0; i < reloaded; i++)
        {
            hack.dots[i].color = sf::Color::White;
        }
        for (size_t i = reloaded; i < 32; i++)
        {
            hack.dots[i].color = sf::Color::Transparent;
        }

        target.draw(dots, states);
    }
    sf::Text text;
    std::wstring hpState;
    sf::CircleShape circle;
    sf::VertexArray polygon;
    sf::VertexArray dots;
    float reloadState;
    bool aimState;
};

#endif
