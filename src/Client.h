#ifndef CLIENT_H
#define CLIENT_H

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <chrono>
#include <string>
#include <future>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <SFML/Graphics.hpp>
#include <SFML/Config.hpp>
#include <SFML/OpenGL.hpp>
#include "ContactListener.h"
#include "Console.h"
#include "Spaceship.h"
#include "Rock.h"
#include "Cursor.h"
#include "ParticleSystem.h"
#include "CommandProcessor.h"
#include "Grid.h"
#include "RenderSerializer.h"
#include "Network.h"
#include "resourceManager.h"
#include "Args.h"
#include "secret.h"
#include "protector.h"
#include "RankedBattle.h"

#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

class GameClient
{
private:
    Args args;

public:
    GameClient(Args &&myArgs) : args(myArgs) {}
    int operator()()
    {
        int port = std::stoi(args["port"]);
        sf::UdpSocket udpSocket;
        std::chrono::high_resolution_clock::time_point now, last = std::chrono::high_resolution_clock::now();
        float delta = 0.f;
        sf::Clock clock1, clockAimCoords;
        sf::Socket::Status socketStatus;
        std::atomic<bool> running = true, mainWantsToEnter = false, drawingWantsToEnter = false;
        std::condition_variable mainCv, drawingCv;
        std::future<void> receivingThread, drawingThread;
        Console console;
        console
            << L"Spaceship commander command prompt\n"
            << L"Client, "
            << L"version from " << CommandProcessor::converter.from_bytes(__DATE__) << L" " << CommandProcessor::converter.from_bytes(__TIME__) << L"\n"
            << L"Use 'help' command to learn the basics.\n\n";

        //Client
        Grid grid;
        Cursor cursor;
        ParticleSystem particleSystem;
        sf::TcpSocket socket;
        sf::Text text;
        DownEvent downEvent, downEventDirectDraw;
        std::vector<DownEvent> downEvents;
        sf::Event event;
        sf::Packet packet;
        bool downEventDirectDrawReceived = false;
        int alive = 0;
        decltype(Object::objects)::iterator i, j;
        constexpr float aimCoordsSignalInterval = 1.f / 80.f;
        float ping = 0.f;
        const float mouseScrollMultiplier = resourceManager::getJSON("config")["mouseScrollMultiplier"].get<float>();
        const int antialiasingLevel = resourceManager::getJSON("config")["antialiasingLevel"].get<int>();
        bool gridVisible = true;
        Vec2f scale;
        std::vector<UpEvent> upEvents;
        std::mutex receivingMutex, drawingMutex, renderTextureMutex;
        std::vector<DownEvent> downEventsBuffer;
        sf::RenderWindow window;
        sf::RenderTexture renderTexture, renderTexture2;
        sf::Image icon;
        sf::Clock pingClock;

        if (socket.connect(args["ip"], port) != sf::Socket::Status::Done)
        {
            sf::sleep(sf::milliseconds(100));
            if (socket.connect(args["ip"], port) != sf::Socket::Status::Done)
            {
                std::cerr << "Could not connect with " << args["ip"] << ":" << port << ".\n";
                std::cerr << "Please try again later." << std::flush;
                return 1;
            }
        }
        if (udpSocket.bind(port + 1) != sf::Socket::Status::Done)
        {
            sf::sleep(sf::milliseconds(100));
            if (udpSocket.bind(port + 1) != sf::Socket::Status::Done)
            {
                std::cerr << "Could not bind udp socket to port " << port + 1 << ".\n";
                std::cerr << "Please try again later." << std::flush;
                return 1;
            }
        }

        Client client(socket, udpSocket);

        const auto fontName = resourceManager::getJSON("config")["fontName"].get<std::string>();
        const auto fontSize = resourceManager::getJSON("config")["fontSize"].get<int>();

        text.setFont(resourceManager::getFont(fontName));
        text.setCharacterSize(fontSize);
        text.setFillColor(sf::Color::White);

        if (args["command"].size())
            upEvents.emplace_back(UpEvent::Type::Command, CommandProcessor::converter.from_bytes(args["command"]));

        receivingThread = std::async(std::launch::async, [&client, &running, &receivingMutex, &downEventsBuffer, &socketStatus, &mainCv, &mainWantsToEnter]() {
            resourceManager::getSoundBuffer("ricochet.ogg");
            resourceManager::getSoundBuffer("explosion.wav");
            sf::Packet packet;
            DownEvent downEvent;
            while (running)
            {
                if (client.wait())
                {
                    std::unique_lock<std::mutex> lk(receivingMutex);
                    while (mainWantsToEnter)
                        mainCv.wait(lk);
                    while ((socketStatus = client.receive(packet)) == sf::Socket::Status::Done)
                    {
                        packet >> downEvent;
                        downEventsBuffer.emplace_back(downEvent);
                        if (mainWantsToEnter)
                            break;
                    }
                }
            }
        });

        icon.loadFromFile("icon.png");
        window.create(sf::VideoMode::getFullscreenModes().front(), "Starship battle", sf::Style::Fullscreen, sf::ContextSettings(0, 0, antialiasingLevel, 3, 3, 0, false));
        window.setVerticalSyncEnabled(resourceManager::getJSON("config")["vSync"].get<bool>());
        window.setMouseCursorVisible(false);
        window.requestFocus();
        window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
        window.setKeyRepeatEnabled(false);

        float lineWidth = resourceManager::getJSON("config")["lineWidth"].get<float>();
        {
            // Intel drivers draws thinner lines compared to Nvidia drivers
            // (at least on my machines)
            std::string vendor(reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
            std::transform(vendor.begin(), vendor.end(), vendor.begin(), [](char c) { return std::toupper(c, std::locale()); });
            if (vendor.find("INTEL") != std::string::npos)
                lineWidth *= 1.3f;
        }
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(lineWidth);
        glPointSize(lineWidth * 1.2f);

        window.setActive(false);
        //sf::err().rdbuf(NULL);

        drawingThread = std::async(std::launch::async, [&running, &drawingMutex, &drawingCv, &drawingWantsToEnter,
                                                        &window, &cursor, &downEventDirectDraw, &console, &scale,
                                                        &text, &alive, &gridVisible, &particleSystem, &lineWidth,
                                                        &grid, &ping, &renderTexture,
                                                        &renderTexture2, &renderTextureMutex]() {
            sf::Sprite renderTextureSprite(renderTexture.getTexture());
            sf::Shader consoleShader, crtShader, blurShader;
            sf::Clock clockFps;
            sf::View temp;
            float fps = 60.f;
            float delta;

            window.setActive(true);

            renderTexture.create(window.getSize().x, window.getSize().y, window.getSettings());
            renderTexture.setSmooth(true);
            renderTexture.setView({{0.f, 0.f}, window.getView().getSize()});

            renderTexture.clear(sf::Color::Green);

            renderTexture2.create(renderTexture.getSize().x, renderTexture.getSize().y, window.getSettings());
            renderTexture2.setSmooth(true);
            renderTexture2.setView(renderTexture.getView());

            if (!consoleShader.loadFromFile("console.vert", "console.frag"))
                std::cerr << "Console shader not loaded." << std::endl;

            if (!crtShader.loadFromFile("crt.frag", sf::Shader::Fragment))
                std::cerr << "Crt shader not loaded." << std::endl;

            if (!blurShader.loadFromFile("blur.frag", sf::Shader::Fragment))
                std::cerr << "Blur shader not loaded." << std::endl;

            while (running)
            {
                {
                    drawingWantsToEnter = true;
                    std::lock_guard<std::mutex> lk(drawingMutex);

                    delta = clockFps.getElapsedTime().asSeconds();
                    clockFps.restart();
                    fps = (19.f * fps + 1.f / delta) / 20.f;

                    window.clear(sf::Color::Magenta);
                    renderTexture.clear(sf::Color::Black);

                    //static sf::View temp;
                    //temp = target.getView();
                    //target.setView(sf::View(sf::FloatRect(0.f , 0.f, (float) target.getSize().x, (float) target.getSize().y)));

                    for (const auto &player : downEventDirectDraw.players)
                    {
                        if (player.id == Object::thisPlayerId)
                        {
                            renderTexture.setView({player.position, renderTexture.getView().getSize()});
                            cursor.setState(player.reload, player.aimState);
                            scale = {renderTexture.getView().getSize().x / (float)renderTexture.getSize().x, renderTexture.getView().getSize().y / (float)renderTexture.getSize().y};
                            break;
                        }
                    }

                    if (!alive)
                    {
                        cursor.setState();
                        scale = {renderTexture.getView().getSize().x / (float)renderTexture.getSize().x, renderTexture.getView().getSize().y / (float)renderTexture.getSize().y};
                    }

                    renderTexture.draw(particleSystem);

                    if (gridVisible)
                    {
                        glLineWidth(lineWidth - 0.2f);
                        renderTexture.draw(grid);
                        glLineWidth(lineWidth);
                    }

                    for (const auto &polygon : downEventDirectDraw.polygons)
                        renderTexture.draw(polygon);

                    for (const auto &player : downEventDirectDraw.players)
                    {
                        if (player.id == Object::thisPlayerId)
                            text.setString(player.playerId + L"\nHp: "s + std::to_wstring(player.hp) + L"/"s + std::to_wstring(player.maxHp) + L"\nSpeed: " + std::to_wstring((int)std::roundf(player.linearVelocity.getLength())));
                        else
                            text.setString(player.playerId + L"\nHp: "s + std::to_wstring(player.hp) + L"/"s + std::to_wstring(player.maxHp));
                        text.setPosition(player.position.x, player.position.y + 500.f / std::max(std::sqrt(scale.y), 2.f));
                        text.setScale(scale);
                        renderTexture.draw(text);
                    }

                    // View = Size block
                    temp = renderTexture.getView();
                    renderTexture.setView(sf::View(sf::FloatRect(0.f, 0.f, renderTexture.getSize().x, renderTexture.getSize().y)));
                    text.setString(L"Fps:  "s + std::to_wstring((int)std::roundf(fps)) + L"\nPing: "s + std::to_wstring((int)std::roundf(ping * 1000.f / 2.f))); // Fps
                    text.setPosition({(float)renderTexture.getSize().x - 106.f, 24.f});
                    text.setScale(1.f, 1.f);
                    renderTexture.draw(text);
                    consoleShader.setParameter("tex", sf::Shader::CurrentTexture); // Console
                    renderTexture.draw(console, &consoleShader);
                    cursor.setPosition(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
                    renderTexture.draw(cursor);
                    renderTexture.setView(temp);

                    drawingWantsToEnter = false;
                }
                drawingCv.notify_one();

                {
                    std::lock_guard<std::mutex> lk(renderTextureMutex);
                    renderTexture.display();
                    renderTexture2.clear(sf::Color::Black);
                    renderTextureSprite.setTexture(renderTexture.getTexture());
                    renderTextureSprite.setScale(scale);
                    renderTextureSprite.setPosition(renderTexture2.mapPixelToCoords({0, 0}));
                    blurShader.setUniform("tex", sf::Shader::CurrentTexture);
                    blurShader.setUniform("dir", sf::Vector2f(1.f, 0.f));
                    renderTexture2.draw(renderTextureSprite, &blurShader);
                    renderTexture2.display();
                    blurShader.setUniform("tex", sf::Shader::CurrentTexture);
                    blurShader.setUniform("dir", sf::Vector2f(0.f, 1.f));
                    renderTextureSprite.setTexture(renderTexture2.getTexture());
                    renderTextureSprite.setPosition(renderTexture.mapPixelToCoords({0, 0}));
                    renderTexture.draw(renderTextureSprite, &blurShader);
                    renderTexture.display();
                }

                crtShader.setParameter("tex", sf::Shader::CurrentTexture);
                window.draw(sf::Sprite(renderTexture.getTexture()), &crtShader);
                window.display();
            }
        });

        std::cout << std::flush;
        std::cerr << std::flush;

        upEvents.emplace_back(UpEvent::Type::Ping);
        pingClock.restart();

        while (window.isOpen())
        {
            std::unique_lock<std::mutex> lk(drawingMutex);
            while (drawingWantsToEnter)
                drawingCv.wait(lk);

            now = std::chrono::high_resolution_clock::now();
            delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - last).count();
            last = now;

            while (window.pollEvent(event))
            {
                //Local events
                switch (event.type)
                {
                case sf::Event::Closed:
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    switch (event.key.code)
                    {
                    case sf::Keyboard::Escape:
                        window.close();
                        break;
                    case sf::Keyboard::Up:
                        console.put(L']');
                        break;
                    case sf::Keyboard::Down:
                        console.put(L'[');
                        break;
                    case sf::Keyboard::Left:
                        console.put(Console::left);
                        break;
                    case sf::Keyboard::Right:
                        console.put(Console::right);
                        break;
                    case sf::Keyboard::C:
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
                            console.put(Console::copy);
                        break;
                    case sf::Keyboard::V:
                        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
                            console.put(Console::paste);
                        break;
                    }
                    break;
                case sf::Event::TextEntered:
                    if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
                        console.put((wchar_t)event.text.unicode);
                    break;
                case sf::Event::Resized:
                {
                    std::lock_guard<std::mutex> lk(renderTextureMutex);
                    renderTexture.create(window.getSize().x, window.getSize().y, window.getSettings());
                    renderTexture.setSmooth(true);
                    renderTexture.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)event.size.width, (float)event.size.height)));

                    renderTexture2.create(renderTexture.getSize().x, renderTexture.getSize().y, window.getSettings());
                    renderTexture2.setSmooth(true);
                    renderTexture2.setView(renderTexture.getView());

                    window.setView(renderTexture.getView());
                    break;
                }
                case sf::Event::MouseWheelScrolled:
                {
                    std::lock_guard<std::mutex> lk(renderTextureMutex);
                    sf::View view = renderTexture.getView();

                    if (event.mouseWheelScroll.delta * mouseScrollMultiplier < 0.f && view.getSize().x / (float)renderTexture.getSize().x + view.getSize().y / (float)renderTexture.getSize().y < 0.1f)
                        break;
                    if (event.mouseWheelScroll.delta * mouseScrollMultiplier > 0.f && view.getSize().x / (float)renderTexture.getSize().x + view.getSize().y / (float)renderTexture.getSize().y > 48.f)
                        break;
                    view.zoom((event.mouseWheelScroll.delta * mouseScrollMultiplier > 0.f) * 1.1f + (event.mouseWheelScroll.delta * mouseScrollMultiplier < 0) * 0.9f);
                    renderTexture.setView(view);
                    renderTexture2.setView(view);
                    break;
                }
                }

                //Player control events
                if (Object::thisPlayerId != -1)
                {
                    switch (event.type)
                    {
                    case sf::Event::KeyPressed:
                    case sf::Event::KeyReleased:
                    {
                        bool ev = event.type == sf::Event::KeyPressed;
                        switch (event.key.code)
                        {
                        case sf::Keyboard::W:
                            upEvents.emplace_back(UpEvent::Type::Forward, Object::thisPlayerId, ev);
                            break;
                        case sf::Keyboard::A:
                            upEvents.emplace_back(UpEvent::Type::Left, Object::thisPlayerId, ev);
                            break;
                        case sf::Keyboard::D:
                            upEvents.emplace_back(UpEvent::Type::Right, Object::thisPlayerId, ev);
                            break;
                        default:
                            break;
                        }
                        break;
                    }
                    case sf::Event::MouseButtonPressed:
                    case sf::Event::MouseButtonReleased:
                        if (event.mouseButton.button == sf::Mouse::Button::Left)
                            upEvents.emplace_back(UpEvent::Type::Shoot, Object::thisPlayerId, event.type == sf::Event::MouseButtonPressed);
                        break;
                    }
                }
            }

            if (console.isTextEntered())
            {
                std::wstring command = console.get();
                if (command.find(L"grid ") == 0 || command.find(L"g ") == 0)
                {
                    if (command.find(L"on") != std::wstring::npos)
                        gridVisible = true;
                    else if (command.find(L"off") != std::wstring::npos)
                        gridVisible = false;
                }
                else if (command.find(L"join-team ") == 0 || command.find(L"jt ") == 0 || command.find(L"team-stats ") == 0 || command.find(L"s ") == 0)
                    upEvents.emplace_back(UpEvent::Type::Command, command + L" " + std::to_wstring(Object::thisPlayerId));
                else
                    upEvents.emplace_back(UpEvent::Type::Command, command);
            }

            if (Object::thisPlayerId != -1 && clockAimCoords.getElapsedTime().asSeconds() >= aimCoordsSignalInterval)
            {
                clockAimCoords.restart();
                upEvents.emplace_back(UpEvent::Type::AimCoords, Object::thisPlayerId, Vec2f::asVec2f(renderTexture.mapPixelToCoords(sf::Mouse::getPosition(window))));

                for (const auto &player : downEventDirectDraw.players)
                    if (player.id == Object::thisPlayerId)
                        alive = 100;

                if (!alive)
                    Object::thisPlayerId = -1;
                else
                    alive--;
            }

            {
                mainWantsToEnter = true;
                std::lock_guard<std::mutex> lk(receivingMutex);
                for (const auto &upEvent : upEvents)
                {
                    packet.clear();
                    packet << upEvent;
                    switch (upEvent.type)
                    {
                    case UpEvent::Type::AimCoords:
                    case UpEvent::Type::Ping:
                        client.sendUdp(packet);
                        break;
                    default:
                        client.send(packet);
                        break;
                    }
                }
                std::swap(downEvents, downEventsBuffer);
                if (socketStatus == sf::Socket::Status::Disconnected)
                {
                    console << L"Connection with server lost.\n";
                    if (clock1.getElapsedTime().asSeconds() >= 1.f)
                    {
                        std::cout << std::flush;
                        break;
                    }
                }
                mainWantsToEnter = false;
            }
            mainCv.notify_one();
            upEvents.clear();

            for (DownEvent &downEvent : downEvents)
            {
                switch (downEvent.type)
                {
                case DownEvent::Type::DirectDraw:
                    downEventDirectDrawReceived = true;
                    downEventDirectDraw = std::move(downEvent);
                    break;
                case DownEvent::Type::Collision:
                    if (downEvent.explosion)
                    {
                        resourceManager::playSound("explosion.wav");
                        particleSystem.impulse(downEvent.collision, downEvent.explosion);
                        if (downEvent.message.size())
                            console << downEvent.message;
                    }
                    else
                        resourceManager::playSound("ricochet.ogg");
                    break;
                case DownEvent::Type::Message:
                    console << downEvent.message;
                    break;
                case DownEvent::Type::Response:
                {
                    std::wstringstream wss;
                    std::wstring temp;
                    wss.str(downEvent.message);
                    while (wss >> temp)
                    {
                        if (temp == L"print"s)
                        {
                            wss.seekg((int)wss.tellg() + 1);
                            std::getline(wss, temp, (wchar_t)std::char_traits<wchar_t>::eof());
                            console << temp;
                        }
                        else if (temp == L"setThisPlayerId"s)
                        {
                            wss >> Object::thisPlayerId;
                            alive = 100;
                        }
                    }
                }
                break;
                case DownEvent::Type::Pong:
                    ping = (19.f * ping + pingClock.getElapsedTime().asSeconds()) / 20.f;
                    pingClock.restart();
                    upEvents.emplace_back(UpEvent::Type::Ping);
                    break;
                }
            }
            downEvents.clear();

            if (downEventDirectDrawReceived)
            {
                downEventDirectDrawReceived = false;

                const float halfPing = ping / 2.f;

                for (auto &polygon : downEventDirectDraw.polygons)
                {
                    polygon.states.transform.rotate(polygon.angularVelocity * halfPing, polygon.position).translate(polygon.linearVelocity * halfPing);
                    polygon.position += polygon.linearVelocity * halfPing;
                }

                for (auto &player : downEventDirectDraw.players)
                    player.position += player.linearVelocity * halfPing;
            }
            else
            {
                for (auto &polygon : downEventDirectDraw.polygons)
                {
                    polygon.states.transform.rotate(polygon.angularVelocity * delta, polygon.position).translate(polygon.linearVelocity * delta);
                    polygon.position += polygon.linearVelocity * delta;
                }

                for (auto &player : downEventDirectDraw.players)
                    player.position += player.linearVelocity * delta;
            }

            particleSystem.update(delta);
            console.update(delta);
            window.setKeyRepeatEnabled(console.isActive());

            if (clock1.getElapsedTime().asSeconds() >= 1.f)
            {
                if (pingClock.getElapsedTime().asSeconds() > 1.f)
                {
                    pingClock.restart();
                    upEvents.emplace_back(UpEvent::Type::Ping);
                }

                clock1.restart();
                std::cout << std::flush;
                std::cerr << std::flush;
            }
        }
        running = false;
        mainWantsToEnter = false;
        mainCv.notify_one();
        if (receivingThread.valid())
            receivingThread.get();
        if (drawingThread.valid())
            drawingThread.get();
        Object::objects.clear();
        return 0;
    }
};

#endif /* CLIENT_H */
