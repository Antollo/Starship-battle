#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <iostream>
#include <chrono>
#include <queue>
#include <future>
#include "Object.h"

class Console : public Object
{
public:
    static Console* create()
    {
        objects.emplace_back(new Console());
        console = dynamic_cast<Console*>(objects.back().get());
        return console;
    }
    /*~Console()
    {
        std::wstringstream s;
        s.str(L"end");
        std::wcin.set_rdbuf(s.rdbuf());
        wcinFuture.wait();
    }*/
    template <class T>
    Console& operator<<(const T& out)
    {
        textOutput.setString(textOutput.getString() + std::to_wstring(out));
        std::wcout << std::to_wstring(out);
        correctOrigin();
        return *this;
    }
    template<>
    Console& operator<<(const std::wstring& out)
    {
        textOutput.setString(textOutput.getString() + out);
        std::wcout << out;
        correctOrigin();
        return *this;
    }
    Console& operator<<(const wchar_t* out)
    {
        textOutput.setString(textOutput.getString() + out);
        std::wcout << out;
        correctOrigin();
        return *this;
    }
    Console& operator>>(std::wstring& in)
    {
        /*if (wcinFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            in = wcinFuture.get();
            wcinFuture = std::async(std::launch::async, []()
            {
                std::wstring in;
                std::getline(std::wcin, in);
                return in;
            });
        }
        else*/
        if (!entered.empty())
        {
            in = entered.front();
            entered.pop();
        }
        else
        {
            in.clear(); 
        }
        return *this;
    }
    std::wstring get()
    {
        if (!entered.empty())
        {
            std::wstring in = entered.front();
            entered.pop();
            return in;
        }
        return L"";
    }
    const Object::typeId getTypeId() const noexcept override
    {
        return Object::typeId::Console;
    }
    const bool isTextEntered() const noexcept
    {
        return !entered.empty() /*|| wcinFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready*/;
    }
    void put(wchar_t c)
    {
        switch (c)
        {
            case '\n':
            case '\r':
                if (!active) break;
                entered.push(textInput.getString().substring(1, textInput.getString().getSize() - 2));
                textOutput.setString(textOutput.getString() + entered.back() + '\n');
                std::wcout << (entered.back() + L"\n");
                textInput.setString(">_");
                correctOrigin();
                break;
            case '\b':
                if (!active) break;
                if (textInput.getString().getSize() > 2)
                    textInput.setString(textInput.getString().substring(0, textInput.getString().getSize() - 2) + '_');
                break;
            case '\t':
                active = !active;
                break;
            default:
                if (!active) break;
                textInput.setString(textInput.getString().substring(0, textInput.getString().getSize() - 1) + c + '_');
                break;
        }
    }
private:
    Console() : active(true)
    {
        font.loadFromFile("UbuntuMono.ttf");
        textOutput.setFont(font);
        textInput.setFont(font);
        textOutput.setCharacterSize(18);
        textInput.setCharacterSize(18);
        textOutput.setFillColor(sf::Color::White);
        textInput.setFillColor(sf::Color::White);
        textInput.setString(">_");
        correctOrigin();
        /*wcinFuture = std::async(std::launch::async, []()
        {
            std::wstring in;
            std::getline(std::wcin, in);
            return in;
        });*/
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept override
    {
        sf::View temp = target.getView();
        target.setView(sf::View(sf::FloatRect(0.f , 0.f, (float) target.getSize().x, (float) target.getSize().y)));
        sf::Vector2f pos(18, -18 + target.getView().getSize().y);
        textOutput.setPosition(pos);
        textInput.setPosition(pos);
        target.draw(textOutput, states);
        target.draw(textInput, states);
        target.setView(temp);
    }
    inline void correctOrigin() noexcept
    {
        textOutput.setOrigin(0, (float) textOutput.getCharacterSize() * (1 + std::count(textOutput.getString().begin(), textOutput.getString().end(), '\n')));
        textInput.setOrigin(0, (float) textInput.getCharacterSize());
    }
    sf::Font font;
    mutable sf::Text textOutput, textInput;
    std::queue<std::wstring> entered;
    //std::future<std::wstring> wcinFuture;
    bool active;
};

#endif