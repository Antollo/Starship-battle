#include <chrono>
#include <SFML\Graphics.hpp>
#include <SFML\OpenGL.hpp>
#include "ContactListener.h"
#include "Console.h"
#include "Spaceship.h"
#include "Rock.h"
#include "Cursor.h"
#include "ParticleSystem.h"
#include "CommandProcessor.h"
#include "Grid.h"
#include "resourceManager.h"


int main()
{
    ParticleSystem::create();
    Console::create();
    Cursor::create();
    CommandProcessor commandProcessor;
    commandProcessor.bind(L"create", [](std::wstring shipType, std::wstring pilotName) {
        Object::spaceship = Spaceship::create(CommandProcessor::converter.to_bytes(shipType), pilotName);
    });
    commandProcessor.bind(L"create-bot", [](std::wstring shipType) {
        Bot::create(CommandProcessor::converter.to_bytes(shipType));
    });
    commandProcessor.bind(L"create-bots", [](std::wstring shipType) {
        std::size_t i = 7;
        while (i--) Bot::create("T-3");
        i = 3;
        while (i--) Bot::create("D");
    });
    commandProcessor.bind(L"credits", [](std::wstring shipType, std::wstring pilotName) {
        (*Object::console) 
        << L"SFML        - www.sfml-dev.org\n"
        << L"Box2D       - box2d.org\n"
        << L"Ubuntu Mono - https://fonts.google.com/specimen/Ubuntu+Mono\n"
        << L"freesound   - freesound.org/people/spaciecat/sounds/456779\n"
        << L"              freesound.org/people/Diboz/sounds/213925\n"
        << L"              freesound.org/people/ryansnook/sounds/110111\n"
        << L"              freesound.org/people/debsound/sounds/437602\n";
    });
    commandProcessor.bind(L"delete", [](std::wstring shipType, std::wstring pilotName) {
        (*Object::console) << L"Monika.chr deleted successfully.\n";
    });
    commandProcessor.bind(L"list-spaceships", [](std::wstring shipType, std::wstring pilotName) {
        (*Object::console) << L"D\nT-3\nTODO: descriptions\n";
    });
    commandProcessor.bind(L"beep", [](std::wstring shipType, std::wstring pilotName) {
        resourceManager::playSound("glitch.wav");
    });
    commandProcessor.bind(L"help", [](std::wstring shipType, std::wstring pilotName) {
        (*Object::console)
        << L"\nSpaceship commander command prompt\n"
        << L"version from " << CommandProcessor::converter.from_bytes(__DATE__) << L" " << CommandProcessor::converter.from_bytes(__TIME__) << L"\n"
        << L"Use W key to accelerate\n"
        << L"Use A and D keys to rotate\n"
        << L"Use mouse right button to aim\n"
        << L"Use mouse left button to shoot\n"
        << L"Use mouse wheel to scale the view\n"
        << L"Use tab to switch console input\n"
        << L"These shell commands are defined internally\n"
        << L"    beep                                \n"
        << L"    create [spaceship type] [pilot name]\n"
        << L"    create-bot [spaceship type]         \n"
        << L"    create-bots                         \n"
        << L"    delete                              \n"
        << L"    TODO: time, heal                    \n"
        << L"    help                                \n"
        << L"    list-spaceships                     \n";
    });
    
    (*Object::console)
    << L"Spaceship commander command prompt\n"
    << L"version from " << CommandProcessor::converter.from_bytes(__DATE__) << L" " << CommandProcessor::converter.from_bytes(__TIME__) << L"\n"
    << L"Use 'help' command to learn the basics.\n\n";
    {
        std::size_t i = 40;
        while (i--) Rock::create();
    }


    sf::RenderWindow window(sf::VideoMode::getFullscreenModes().front(), "My window", sf::Style::Fullscreen, sf::ContextSettings(0, 0, 16, 1, 1, 0, false));
    //sf::RenderWindow window(sf::VideoMode::getFullscreenModes().back(), "My window", sf::Style::Default, sf::ContextSettings(0, 0, 16, 1, 1, 0, false));
    window.setVerticalSyncEnabled(true);

    //sf::Shader shader;
    //shader.loadFromFile("vertex_shader.vert", "fragment_shader.frag");
    //shader.setUniform("texture", sf::Shader::CurrentTexture);
    //shader.setUniform("offsetFactor", sf::Vector2f(-0.0005f, 0.f));
    //sf::Texture gaussianBlurredTexture;
    //gaussianBlurredTexture.create(sf::VideoMode::getFullscreenModes().front().width, sf::VideoMode::getFullscreenModes().front().height);
    //sf::Sprite gaussianBlur(gaussianBlurredTexture);

    Grid grid;


    std::chrono::high_resolution_clock::time_point now, last = std::chrono::high_resolution_clock::now();
    ContactListener contactListener;
    Object::world.SetContactListener(&contactListener);
    sf::Event event;
    float delta = 0.f;

    resourceManager::playSound("glitch.wav");

    while (window.isOpen())
    {
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
                if (event.key.code == sf::Keyboard::Escape)
                    window.close();
            if (event.type == sf::Event::TextEntered)
                Object::console->put((wchar_t)event.text.unicode);
            if (event.type == sf::Event::Resized)
                window.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)event.size.width, (float)event.size.height)));
            if (event.type == sf::Event::MouseWheelScrolled)
            {
                sf::View view = window.getView();
                if (event.mouseWheelScroll.delta < 0.f && view.getSize().x/(float)window.getSize().x + view.getSize().y/(float)window.getSize().y < 0.25f)
                    break;
                if (event.mouseWheelScroll.delta > 0.f && view.getSize().x/(float)window.getSize().x + view.getSize().y/(float)window.getSize().y > 32.f)
                    break;
                view.zoom((event.mouseWheelScroll.delta > 0.f) * 1.2f + (event.mouseWheelScroll.delta < 0) * 0.8f);
                window.setView(view);
            }
            if (Object::spaceship != nullptr)
            {
                if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased)
                {
                    bool ev = event.type == sf::Event::KeyPressed;
                    switch (event.key.code)
                    {
                        case sf::Keyboard::W:
                            Object::spaceship->forward = ev;
                            break;
                        case sf::Keyboard::A:
                            Object::spaceship->left = ev;
                            break;
                        case sf::Keyboard::D:
                            Object::spaceship->right = ev;
                            break;
                        default:
                            break;
                    }
                }
                if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased)
                {
                    bool ev = event.type == sf::Event::MouseButtonPressed;
                    switch (event.mouseButton.button)
                    {
                        case sf::Mouse::Button::Right:
                            Object::spaceship->aim = ev;
                            break;
                        case sf::Mouse::Button::Left:
                            Object::spaceship->shoot = ev;
                            break;
                        default:
                            Object::spaceship->aimCoords = Vec2f::asVec2f(window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y}));
                            break;
                    }
                }
                if (event.type == sf::Event::MouseMoved)
                    Object::spaceship->aimCoords = Vec2f::asVec2f(window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y}));
            }
        }

        if (Object::console->isTextEntered())
            commandProcessor.call(Object::console->get());

		for (auto it = Object::objects.begin(); it != Object::objects.end();it++)
			(*it)->process();

        now = std::chrono::high_resolution_clock::now();
        delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - last).count();
        last = now;
        Object::world.Step(delta, 8, 3);

        if (Object::particleSystem != nullptr)
            Object::particleSystem->update(delta);

        if (Object::spaceship != nullptr)
            window.setView({Object::spaceship->getCenterPosition(), window.getView().getSize()});
        
        window.clear(sf::Color::Black);
        for (const auto& object : Object::objects)
            window.draw(*object);
        window.draw(grid);
        //gaussianBlurredTexture.update(window);
        //window.draw(gaussianBlur, &shader);
        window.display();
    }
	Object::objects.clear();

    return 0;
}