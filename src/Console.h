#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <iostream>
#include <chrono>
#include <queue>
#include <future>
#include "Object.h"
#include "resourceManager.h"

class Console : public sf::Drawable
{
public:
    /*static Console* create()
    {
        console = dynamic_cast<Console*>(objects.emplace(counter, new Console()).first->second.get());
        return console;
    }
    ~Console()
    {
        std::wstringstream s;
        s.str(L"end");
        std::wcin.set_rdbuf(s.rdbuf());
        wcinFuture.wait();
    }*/
    Console() : active(true), history({L""}), historyIterator(0)
    {
        textOutput.setFont(resourceManager::getFont("UbuntuMono.ttf"));
        textInput.setFont(resourceManager::getFont("UbuntuMono.ttf"));
        notificationOutput.setFont(resourceManager::getFont("UbuntuMono.ttf"));
        textOutput.setCharacterSize(18);
        textInput.setCharacterSize(18);
        notificationOutput.setCharacterSize(18);
        textOutput.setFillColor(sf::Color::White);
        textInput.setFillColor(sf::Color::White);
        notificationOutput.setFillColor(sf::Color::White);
        textInput.setString(">_");
        correctOrigin();
        /*wcinFuture = std::async(std::launch::async, []()
        {
            std::wstring in;
            std::getline(std::wcin, in);
            return in;
        });*/
    }
    template <class T>
    Console& operator<<(const T& out)
    {
        textOutput.setString(textOutput.getString() + std::to_wstring(out));
        notificationOutput.setString(std::to_wstring(out));
        std::wcout << std::to_wstring(out);
        correctOrigin();
        return *this;
    }
    Console& operator<<(const std::wstring& out)
    {
        textOutput.setString(textOutput.getString() + out);
        notificationOutput.setString(out);
        std::wcout << out;
        correctOrigin();
        return *this;
    }
    Console& operator<<(const wchar_t* out)
    {
        textOutput.setString(textOutput.getString() + out);
        notificationOutput.setString(out);
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
    /*const Object::TypeId getTypeId() const noexcept override
    {
        return Object::TypeId::Console;
    }*/
    bool isTextEntered() const noexcept
    {
        return !entered.empty() /*|| wcinFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready*/;
    }
    void put(wchar_t c)
    {
        switch (c)
        {
            case L'\n':
            case L'\r':
                if (!active) break;
                entered.push(textInput.getString().substring(1, textInput.getString().getSize() - 2));
                history.insert(history.begin() + 1, entered.back());
                textOutput.setString(textOutput.getString() + entered.back() + '\n');
                std::wcout << (entered.back() + L"\n");
                textInput.setString(L">_");
                correctOrigin();
                break;
            case L'\b':
                if (!active) break;
                if (textInput.getString().getSize() > 2)
                    textInput.setString(textInput.getString().substring(0, textInput.getString().getSize() - 2) + '_');
                break;
            case L'`':
            case L'~':
                active = !active;
                break;
            case L'\t':
                //Autocomplete
                if (!active) break;
                it = std::find_if(history.begin(), history.end(),
                [key = textInput.getString().substring(1, textInput.getString().getSize() - 2)](const std::wstring& str) {
                    return str.find(key) == 0;
                });
                if (it != history.end())
                    textInput.setString(L">"s + *it + L"_"s);
                break;
            case L']':
                if (!active) break;
                historyIterator++;
                historyIterator %= history.size();
                textInput.setString(L">"s + history[historyIterator] + L"_"s);
                break;
            case L'[':
                if (!active) break;
                if (historyIterator) historyIterator--;
                textInput.setString(L">"s + history[historyIterator] + L"_"s);
                break;
            default:
                if (!active) break;
                textInput.setString(textInput.getString().substring(0, textInput.getString().getSize() - 1) + c + '_');
                break;
        }
    }
private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const noexcept override
    {
        static sf::View temp;
        static sf::Vector2f pos(18.f, 0.f);

        temp = target.getView();
        target.setView(sf::View(sf::FloatRect(0.f , 0.f, (float) target.getSize().x, (float) target.getSize().y)));
        pos.y = -18 + target.getView().getSize().y;
        if (!active) 
        {
            notificationOutput.setPosition(pos);
            target.draw(notificationOutput, states);
            target.setView(temp);
            return;
        }
        textOutput.setPosition(pos);
        textInput.setPosition(pos);
        target.draw(textOutput, states);
        target.draw(textInput, states);
        target.setView(temp);
    }
    inline void correctOrigin() noexcept
    {
        notificationOutput.setOrigin(0, (float) notificationOutput.getCharacterSize() * (1 + std::count(notificationOutput.getString().begin(), notificationOutput.getString().end(), '\n')));
        textOutput.setOrigin(0, (float) textOutput.getCharacterSize() * (1 + std::count(textOutput.getString().begin(), textOutput.getString().end(), '\n')));
        textInput.setOrigin(0, (float) textInput.getCharacterSize());
    }
    mutable sf::Text textOutput, notificationOutput, textInput;
    std::queue<std::wstring> entered;
    std::deque<std::wstring> history;
    std::deque<std::wstring>::iterator it;
    std::size_t historyIterator;
    //std::future<std::wstring> wcinFuture;
    bool active;
};

#endif
