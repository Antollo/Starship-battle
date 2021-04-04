#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <iostream>
#include <chrono>
#include <queue>
#include <list>
#include <future>
#include <SFML/Window/Clipboard.hpp>
#include "Object.h"
#include "resourceManager.h"

class Console : public sf::Drawable
{
public:
    static constexpr wchar_t up = L']';
    static constexpr wchar_t down = L'[';
    static constexpr wchar_t left = 2;
    static constexpr wchar_t right = 3;
    static constexpr wchar_t copy = 4;
    static constexpr wchar_t paste = 5;

    Console()
    {
        const auto fontName = resourceManager::getJSON("config")["fontName"].get<std::string>();
        const auto fontSize = resourceManager::getJSON("config")["fontSize"].get<int>();

        glyphWidth = resourceManager::getFont(fontName).getGlyph(L'a', fontSize, false).advance;

        textOutput.setFont(resourceManager::getFont(fontName));
        textInput.setFont(resourceManager::getFont(fontName));
        notificationOutput.setFont(resourceManager::getFont(fontName));
        textPrompt.setFont(resourceManager::getFont(fontName));
        textCursor.setFont(resourceManager::getFont(fontName));

        textOutput.setCharacterSize(fontSize);
        textInput.setCharacterSize(fontSize);
        notificationOutput.setCharacterSize(fontSize);
        textPrompt.setCharacterSize(fontSize);
        textCursor.setCharacterSize(fontSize);

        textOutput.setFillColor(sf::Color::White);
        textInput.setFillColor(sf::Color::White);
        notificationOutput.setFillColor(sf::Color::White);
        textPrompt.setFillColor(sf::Color::White);
        textCursor.setFillColor(sf::Color::White);

        textPrompt.setString(prompt);
        textCursor.setString(cursor);

        correctOrigin();

        autocompleteIterator = history.begin();
    }
    template <class T>
    Console &operator<<(const T &out)
    {
        textOutput.setString(textOutput.getString() + std::to_wstring(out));
        notificationOutput.setString(std::to_wstring(out));
        eraseOldOut();
        correctOrigin();
        return *this;
    }
    Console &operator<<(const std::wstring &out)
    {
        textOutput.setString(textOutput.getString() + out);
        notificationOutput.setString(out);
        eraseOldOut();
        correctOrigin();
        return *this;
    }
    Console &operator<<(const wchar_t *out)
    {
        textOutput.setString(textOutput.getString() + out);
        notificationOutput.setString(out);
        eraseOldOut();
        correctOrigin();
        return *this;
    }
    Console &operator>>(std::wstring &in)
    {
        if (!entered.empty())
        {
            in = entered.front();
            entered.pop();
        }
        else
            in.clear();
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
    bool isTextEntered() const
    {
        return !entered.empty();
    }
    void put(wchar_t c)
    {
        std::wstring string = textInput.getString();

        switch (c)
        {
        case L'\n':
        case L'\r':
            if (!active)
                break;
            entered.push(string);
            history.insert(history.begin() + 1, entered.back());
            textOutput.setString(textOutput.getString() + string + '\n');
            textInput.setString(L"");
            inputIterator = 0;
            correctOrigin();
            break;
        case L'\b':
            if (!active)
                break;
            if (inputIterator)
            {
                inputIterator--;
                string.erase(inputIterator, 1);
                textInput.setString(string);
            }
            break;
        case L'`':
        case L'~':
            active = !active;
            break;
        case L'\t':
            //Autocomplete
            if (!active)
                break;
            if (autocompleteIterator == history.end())
                autocompleteIterator = history.begin();
            autocompleteIterator = std::find_if(autocompleteIterator + 1, history.end(),
                                                [this](const std::wstring &str) {
                                                    return str.find(autocompleteString) == 0;
                                                });
            if (autocompleteIterator != history.end())
            {
                textInput.setString(*autocompleteIterator);
                inputIterator = textInput.getString().getSize();
            }
            else
                autocompleteIterator = history.begin();
            break;
        case up:
            if (!active)
                break;
            historyIterator++;
            historyIterator = std::min(historyIterator, history.size() - 1);
            textInput.setString(history[historyIterator]);
            inputIterator = textInput.getString().getSize();
            break;
        case down:
            if (!active)
                break;
            if (historyIterator)
                historyIterator--;
            textInput.setString(history[historyIterator]);
            inputIterator = textInput.getString().getSize();
            break;
        case left:
            if (!active)
                break;
            if (inputIterator)
                inputIterator--;
            break;
        case right:
            if (!active)
                break;
            inputIterator++;
            inputIterator = std::min(inputIterator, string.size());
            break;
        case paste:
            string.insert(inputIterator, sf::Clipboard::getString().toWideString().c_str());
            textInput.setString(string);
            inputIterator += sf::Clipboard::getString().getSize();
        break;
        case copy:
            sf::Clipboard::setString(textInput.getString());
            break;
        default:
            if (!active)
                break;
            if (string.size() > 32 && string[string.size() - 1] == string[string.size() - 2] && string[string.size() - 2] == string[string.size() - 3] && string.find(' ') == std::wstring::npos)
            {
                active = false;
                textInput.setString(L"");
                inputIterator = 0;
                break;
            }
            string.insert(inputIterator, 1, c);
            inputIterator++;
            textInput.setString(string);
            break;
        }

        if (c != '\t')
        {
            autocompleteIterator = history.begin();
            autocompleteString = textInput.getString();
        }
    }
    void update(float elapsed)
    {
        time += elapsed;
        if (time > 0.6f)
        {
            time = 0.f;
            blink = !blink;
        }
    }
    bool isActive() const { return active; }

private:
    void draw(sf::RenderTarget &target, sf::RenderStates states) const noexcept override
    {
        sf::Vector2f pos(24.f, -24 + target.getView().getSize().y);

        if (!active)
        {
            notificationOutput.setPosition(pos);
            target.draw(notificationOutput, states);
            return;
        }
        textOutput.setPosition(pos);
        textPrompt.setPosition(pos);
        textInput.setPosition({pos.x + glyphWidth * 2, pos.y});
        textCursor.setPosition({pos.x + glyphWidth * 2 + glyphWidth * inputIterator, pos.y});
        target.draw(textOutput, states);
        target.draw(textInput, states);
        target.draw(textPrompt, states);
        if (blink)
            target.draw(textCursor, states);
    }
    inline void correctOrigin() noexcept
    {
        notificationOutput.setOrigin(0, (float)notificationOutput.getCharacterSize() * (1 + std::count(notificationOutput.getString().begin(), notificationOutput.getString().end(), '\n')));
        textOutput.setOrigin(0, (float)textOutput.getCharacterSize() * (1 + std::count(textOutput.getString().begin(), textOutput.getString().end(), '\n')));
        textInput.setOrigin(0, (float)textInput.getCharacterSize());
        textPrompt.setOrigin(0, (float)textPrompt.getCharacterSize());
        textCursor.setOrigin(0, (float)textCursor.getCharacterSize());
    }
    void eraseOldOut()
    {
        if (textOutput.getString().getSize() > 5000)
            textOutput.setString(textOutput.getString().substring(textOutput.getString().getSize() - 3000));
    }

    mutable sf::Text textOutput, notificationOutput, textInput, textPrompt, textCursor;
    std::queue<std::wstring> entered;

    std::deque<std::wstring> history = {L""};
    std::deque<std::wstring>::iterator autocompleteIterator;
    std::wstring autocompleteString;

    std::size_t historyIterator = 0;
    size_t inputIterator = 0;

    static inline const sf::String cursor = sf::String((sf::Uint32)9608);
    static inline const sf::String prompt = sf::String("$");
    float glyphWidth;
    float time = 0.f;
    bool active = true;
    bool blink = false;
};

#endif
