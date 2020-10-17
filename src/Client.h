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
        sf::Clock clock1, clockFps, clock2Fps;
        float fps = 100.f;
        sf::Socket::Status socketStatus;
        std::atomic<bool> running = true, mainWantsToEnter = false;
        std::condition_variable cv;
        std::future<void> receivingThread;
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
        constexpr float recommendedDelta = 1.f / 80.f;
        float ping = 0.f, mouseScrollMultiplier = 1.f, temp;
        int antialiasingLevel = 16;
        bool gridVisible = true;
        Vec2f scale;
        std::vector<UpEvent> upEvents;
        std::mutex clientMutex;
        std::vector<DownEvent> downEventsBuffer;
        sf::RenderWindow window; //Graphic window
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

        receivingThread = std::async(std::launch::async, [&client, &running, &clientMutex, &downEventsBuffer, &socketStatus, &cv, &mainWantsToEnter]() {
            resourceManager::getSoundBuffer("ricochet.ogg");
            resourceManager::getSoundBuffer("explosion.wav");
            sf::Packet packet;
            DownEvent downEvent;
            while (running)
            {
                if (client.wait())
                {
                    std::unique_lock<std::mutex> lk(clientMutex);
                    while (mainWantsToEnter)
                        cv.wait(lk);
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
        window.setVerticalSyncEnabled(false);
        window.setMouseCursorVisible(false);
        window.requestFocus();
        window.setView({{0.f, 0.f}, window.getView().getSize()});
        window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
        window.setKeyRepeatEnabled(false);

        const float lineWidth = resourceManager::getJSON("config")["lineWidth"].get<float>();
        glLineWidth(lineWidth);
        glPointSize(lineWidth);
        glEnable(GL_POINT_SMOOTH);

        std::cout << std::flush;
        std::cerr << std::flush;

        upEvents.emplace_back(UpEvent::Type::Ping);
        pingClock.restart();

        while (window.isOpen())
        {
            now = std::chrono::high_resolution_clock::now();
            delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - last).count();
            last = now;

            while (window.pollEvent(event))
            {
                //Local events
                if (event.type == sf::Event::Closed)
                    window.close();
                if (event.type == sf::Event::KeyPressed)
                {
                    if (event.key.code == sf::Keyboard::Escape)
                        window.close();
                    else if (event.key.code == sf::Keyboard::Up)
                        console.put(L']');
                    else if (event.key.code == sf::Keyboard::Down)
                        console.put(L'[');
                }
                if (event.type == sf::Event::TextEntered)
                    console.put((wchar_t)event.text.unicode);
                if (event.type == sf::Event::Resized)
                    window.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)event.size.width, (float)event.size.height)));
                if (event.type == sf::Event::MouseWheelScrolled)
                {
                    sf::View view = window.getView();

                    if (event.mouseWheelScroll.delta * mouseScrollMultiplier < 0.f && view.getSize().x / (float)window.getSize().x + view.getSize().y / (float)window.getSize().y < 0.1f)
                        break;
                    if (event.mouseWheelScroll.delta * mouseScrollMultiplier > 0.f && view.getSize().x / (float)window.getSize().x + view.getSize().y / (float)window.getSize().y > 48.f)
                        break;
                    view.zoom((event.mouseWheelScroll.delta * mouseScrollMultiplier > 0.f) * 1.1f + (event.mouseWheelScroll.delta * mouseScrollMultiplier < 0) * 0.9f);
                    window.setView(view);
                }
                //Player control events
                if (Object::thisPlayerId != -1)
                {
                    if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased)
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
                    }
                    if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased)
                    {
                        bool ev = event.type == sf::Event::MouseButtonPressed;
                        switch (event.mouseButton.button)
                        {
                        case sf::Mouse::Button::Left:
                            upEvents.emplace_back(UpEvent::Type::Shoot, Object::thisPlayerId, ev);
                            break;
                        }
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

            if (Object::thisPlayerId != -1 && clock2Fps.getElapsedTime().asSeconds() >= recommendedDelta * 1.5f)
            {
                clock2Fps.restart();
                upEvents.emplace_back(UpEvent::Type::AimCoords, Object::thisPlayerId, Vec2f::asVec2f(window.mapPixelToCoords(sf::Mouse::getPosition(window))));
            }

            {
                mainWantsToEnter = true;
                std::lock_guard<std::mutex> lk(clientMutex);
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
            cv.notify_one();
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
                    ping = (49.f * ping + pingClock.getElapsedTime().asSeconds()) / 50.f;
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

            particleSystem.update(delta);

            if ((temp = clockFps.getElapsedTime().asSeconds()) >= recommendedDelta)
            {
                clockFps.restart();
                fps = (49.f * fps + 1.f / temp) / 50.f;
                
                window.clear(sf::Color::Black);

                for (const auto &player : downEventDirectDraw.players)
                {
                    if (player.id == Object::thisPlayerId)
                    {
                        alive = 100;
                        window.setView({player.position, window.getView().getSize()});
                        cursor.setState(player.reload, player.aimState, player.hp, player.maxHp);
                        scale = {window.getView().getSize().x / (float)window.getSize().x, window.getView().getSize().y / (float)window.getSize().y};
                        text.setString(player.playerId + L"\nHp: "s + std::to_wstring(player.hp) + L"/"s + std::to_wstring(player.maxHp) + L"\nSpeed: " + std::to_wstring((int)std::roundf(player.linearVelocity.getLength())));
                    }
                    else
                        text.setString(player.playerId + L"\nHp: "s + std::to_wstring(player.hp) + L"/"s + std::to_wstring(player.maxHp));
                    text.setPosition(player.position.x, player.position.y + 500.f / std::max(std::sqrt(scale.y), 2.f));
                    text.setScale(scale);
                    window.draw(text);
                }
                if (!alive)
                {
                    Object::thisPlayerId = -1;
                    cursor.setState();
                    scale = {window.getView().getSize().x / (float)window.getSize().x, window.getView().getSize().y / (float)window.getSize().y};
                }
                else
                    alive--;

                for (const auto &polygon : downEventDirectDraw.polygons)
                    window.draw(polygon.vertices, polygon.states);

                text.setString(L"Fps:  "s + std::to_wstring((int)std::roundf(fps)) 
                    + L"\nPing: "s + std::to_wstring((int)std::roundf(ping * 1000.f / 2.f)));
                text.setScale(scale);
                text.setPosition(window.mapPixelToCoords({(int)window.getSize().x - 100, 18}));

                window.draw(text);
                window.draw(cursor);
                window.draw(particleSystem);
                window.draw(console);
                if (gridVisible)
                {
                    glLineWidth(lineWidth * 0.6f);
                    window.draw(grid);
                    glLineWidth(lineWidth);
                }
                window.display();
            }

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
        mainWantsToEnter = false;
        running = false;
        cv.notify_one();
        if (receivingThread.valid())
            receivingThread.get();
        Object::objects.clear();
        return 0;
    }
};

#endif /* CLIENT_H */
