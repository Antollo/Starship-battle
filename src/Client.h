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
        std::atomic<bool> running = true, mainWantsToEnter = false, drawingWantsToEnter = false, logicFrameDone = false;
        std::condition_variable mainCv, drawingCv;
        std::future<void> receivingThread, drawingThread;
        Console console; //This thing displaying text ui
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
        float ping = 0.f, mouseScrollMultiplier = 1.f, temp;
        int antialiasingLevel = 16;
        bool gridVisible = true;
        Vec2f scale;
        std::vector<UpEvent> upEvents;
        std::mutex receivingMutex, drawingMutex;
        std::vector<DownEvent> downEventsBuffer;
        sf::RenderWindow window;
        sf::Image icon;
        sf::Clock pingClock;

        if (socket.connect(args["ip"], port) != sf::Socket::Status::Done)
        {
            if (socket.connect(args["ip"], port) != sf::Socket::Status::Done)
            {
                std::cerr << "Could not connect with " << args["ip"] << ":" << port << ".\n";
                std::cerr << "Please try again later." << std::flush;
                return 1;
            }
        }
        if (udpSocket.bind(port + 1) != sf::Socket::Status::Done)
        {
            if (udpSocket.bind(port + 1) != sf::Socket::Status::Done)
            {
                std::cerr << "Could not bind udp socket to port " << port + 1 << ".\n";
                std::cerr << "Please try again later." << std::flush;
                return 1;
            }
        }

        Client client(socket, udpSocket);

        text.setFont(resourceManager::getFont("UbuntuMono.ttf"));
        text.setCharacterSize(20);
        text.setFillColor(sf::Color::White);

        std::ifstream file("config.json");
        if (file.good())
        {
            json jsonObject = json::parse(file);
            mouseScrollMultiplier = jsonObject["mouseScrollMultiplier"].get<float>();
            antialiasingLevel = jsonObject["antialiasingLevel"].get<int>();
        }

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
        window.setVerticalSyncEnabled(true);
        window.setMouseCursorVisible(false);
        window.requestFocus();
        window.setView({{0.f, 0.f}, window.getView().getSize()});
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
        sf::err().rdbuf(NULL);

        drawingThread = std::async(std::launch::async, [&running, &drawingMutex, &drawingCv, &drawingWantsToEnter,
                                                        &window, &cursor, &downEventDirectDraw, &console, &scale,
                                                        &text, &alive, &gridVisible, &particleSystem, &lineWidth,
                                                        &grid, &ping, &logicFrameDone]() {
            window.setActive(true);
            sf::Clock clockFps;
            float fps = 100.f;
            float delta;

            while (running)
            {
                {
                    drawingWantsToEnter = true;
                    std::lock_guard<std::mutex> lk(drawingMutex);

                    logicFrameDone = false;

                    delta = clockFps.getElapsedTime().asSeconds();
                    clockFps.restart();
                    fps = (19.f * fps + 1.f / delta) / 20.f;

                    window.clear(sf::Color::Black);

                    for (const auto &player : downEventDirectDraw.players)
                    {
                        if (player.id == Object::thisPlayerId)
                        {
                            window.setView({player.position, window.getView().getSize()});
                            cursor.setState(player.reload, player.aimState, player.hp, player.maxHp);
                            scale = {window.getView().getSize().x / (float)window.getSize().x, window.getView().getSize().y / (float)window.getSize().y};
                        }
                    }

                    if (!alive)
                    {
                        cursor.setState();
                        scale = {window.getView().getSize().x / (float)window.getSize().x, window.getView().getSize().y / (float)window.getSize().y};
                    }

                    window.draw(particleSystem);

                    if (gridVisible)
                    {
                        glLineWidth(lineWidth - 0.5f);
                        window.draw(grid);
                        glLineWidth(lineWidth);
                    }

                    for (const auto &polygon : downEventDirectDraw.polygons)
                        window.draw(polygon);

                    for (const auto &player : downEventDirectDraw.players)
                    {
                        if (player.id == Object::thisPlayerId)
                            text.setString(player.playerId + L"\nHp: "s + std::to_wstring(player.hp) + L"/"s + std::to_wstring(player.maxHp) + L"\nSpeed: " + std::to_wstring((int)std::roundf(player.linearVelocity.getLength())));
                        else
                            text.setString(player.playerId + L"\nHp: "s + std::to_wstring(player.hp) + L"/"s + std::to_wstring(player.maxHp));
                        text.setPosition(player.position.x, player.position.y + 500.f / std::max(std::sqrt(scale.y), 2.f));
                        text.setScale(scale);
                        window.draw(text);
                    }

                    text.setString(L"Fps:  "s + std::to_wstring((int)std::roundf(fps)) + L"\nPing: "s + std::to_wstring((int)std::roundf(ping * 1000.f / 2.f)));
                    text.setScale(scale);
                    text.setPosition(window.mapPixelToCoords({(int)window.getSize().x - 100, 18}));

                    window.draw(text);
                    window.draw(cursor);
                    window.draw(console);

                    drawingWantsToEnter = false;
                }
                drawingCv.notify_one();

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
            while (drawingWantsToEnter && logicFrameDone)
                drawingCv.wait(lk);

            logicFrameDone = true;

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
                    }
                    break;
                case sf::Event::TextEntered:
                    console.put((wchar_t)event.text.unicode);
                    break;
                case sf::Event::Resized:
                    window.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)event.size.width, (float)event.size.height)));
                    break;
                case sf::Event::MouseWheelScrolled:
                {
                    sf::View view = window.getView();

                    if (event.mouseWheelScroll.delta * mouseScrollMultiplier < 0.f && view.getSize().x / (float)window.getSize().x + view.getSize().y / (float)window.getSize().y < 0.1f)
                        break;
                    if (event.mouseWheelScroll.delta * mouseScrollMultiplier > 0.f && view.getSize().x / (float)window.getSize().x + view.getSize().y / (float)window.getSize().y > 48.f)
                        break;
                    view.zoom((event.mouseWheelScroll.delta * mouseScrollMultiplier > 0.f) * 1.1f + (event.mouseWheelScroll.delta * mouseScrollMultiplier < 0) * 0.9f);
                    window.setView(view);
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
                upEvents.emplace_back(UpEvent::Type::AimCoords, Object::thisPlayerId, Vec2f::asVec2f(window.mapPixelToCoords(sf::Mouse::getPosition(window))));

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

            for (auto &el : downEvents)
            {
                DownEvent &downEvent = el;
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
                        particleSystem.impulse(downEvent.collision);
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
