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
#include "PerformanceLevels.h"

#define SFML_DEFINE_DISCRETE_GPU_PREFERENCE

int main(int argc, const char *argv[])
{
    static_assert(sizeof(b2Vec2) == sizeof(Vec2f));
    static_assert(sizeof(sf::Vector2f) == sizeof(Vec2f));

    // Client and server
    Args args(argc, argv);
    bool serverSide = (args["server"].size());
    int port = std::stoi(args["port"]);
    sf::UdpSocket udpSocket;
    std::chrono::high_resolution_clock::time_point now, last = std::chrono::high_resolution_clock::now();
    float delta = 0.f;
    sf::Clock clock1, clock_f;
    int velocityIterations = 8, positionIterations = 3, ticks = 0;
    float fps = 100.f;
    sf::Socket::Status socketStatus;
    std::atomic<bool> running = true, mainWantsToEnter = false;
    std::condition_variable cv;
    std::future<void> receivingThread;
    Console console; //This thing displaying text ui
    console
        << L"Spaceship commander command prompt\n"
        << (serverSide ? L"Server, " : L"Client, ")
        << L"version from " << CommandProcessor::converter.from_bytes(__DATE__) << L" " << CommandProcessor::converter.from_bytes(__TIME__) << L"\n"
        << L"Use 'help' command to learn the basics.\n\n";

    //Client
    Grid grid;
    Cursor cursor;
    ParticleSystem particleSystem;
    sf::TcpSocket socket;
    sf::Text text;
    DownEvent downEvent, downEventDirectDraw;
    bool downEventDirectDrawReceived = false;
    int alive = 0;
    decltype(Object::objects)::iterator i, j;
    float mouseScrollMultiplier = 1.f, ping = 0.f;
    int antialiasingLevel = 16;
    Vec2f scale;
    std::vector<UpEvent> upEvents;
    std::mutex clientMutex;
    std::vector<DownEvent> downEventsBuffer;
    sf::RenderWindow window; //Graphic window
    sf::Image icon;
    sf::Clock pingClock;

    if (!serverSide)
    {
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
    }
    Client client(socket, udpSocket);
    if (!serverSide)
    {
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
        window.create(sf::VideoMode::getFullscreenModes().front(), "Starship battle", sf::Style::Fullscreen, sf::ContextSettings(0, 0, antialiasingLevel, 1, 1, 0, false));
        window.setVerticalSyncEnabled(false);
        window.setFramerateLimit(60);
        window.setMouseCursorVisible(false);
        window.requestFocus();
        window.setView({{0.f, 0.f}, window.getView().getSize()});
        window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

        glLineWidth(resourceManager::getJSON("config")["lineWidth"].get<float>());
        glPointSize(resourceManager::getJSON("config")["lineWidth"].get<float>());
    }

    //Server
    int upEventCounter = 0;
    CommandProcessor commandProcessor; //For scripting in console (commands are run on server side)
    sf::TcpListener listener;          //Server
    if (serverSide)
    {
        listener.listen(port);
        if (udpSocket.bind(port) != sf::Socket::Status::Done)
        {
            if (udpSocket.bind(port) != sf::Socket::Status::Done)
            {
                std::cerr << "Could not bind udp socket to port " << port << ".\n";
                std::cerr << "Please try again later." << std::flush;
                return 1;
            }
        }
    }
    Server server(listener, udpSocket);
    RenderSerializer renderSerializer; //"Display" to Packet
    UpEvent upEvent;
    std::vector<DownEvent> downEvents;
    std::vector<std::pair<UpEvent, Server::iterator>> upEventsRespondable, upEventsRespondableBuffer;
    std::mutex serverMutex;
    PerformanceLevels::Id performanceLevelId = PerformanceLevels::Id::Normal;
    ContactListener contactListener(downEvents);
    Object::world.SetContactListener(&contactListener);
    b2ThreadPoolTaskExecutor executor;
    sf::Event event;
    sf::Packet packet;
    int lastActiveCounter;
    constexpr const auto secret = obfuscate(SECRET);
    commandProcessor.bind(L"ranked", [&commandProcessor, &downEvents, &secret]() {
        static RankedBattle<decltype(obfuscate(SECRET))> rankedBattle(commandProcessor, downEvents, secret);
        Object::destroyAll();
        commandProcessor.bind(L"create-ranked", [&commandProcessor, &downEvents, &secret](std::wstring shipType, std::wstring pilotName) {
            Object::ObjectId id = rankedBattle.start(shipType, pilotName);
            if (id)
                return L"setThisPlayerId "s + std::to_wstring(id) + L" print Spaceship for ranked battle is ready.\n"s;
            return L"print Debugger detected.\n"s;
        });
        return L"print Ranked battle mode.\nUse 'create-ranked [spaceship type] [pilot name]' to start ranked battle.\n\n"s;
    });

    if (serverSide)
    {
        std::size_t n = 60;
        while (n--)
            Rock::create();

        receivingThread = std::async(std::launch::async, [&server, &running, &serverMutex, &upEventsRespondableBuffer, &cv, &mainWantsToEnter]() {
            sf::Packet packet;
            Server::iterator it;
            UpEvent upEvent;
            while (running)
            {
                {
                    std::unique_lock<std::mutex> lk(serverMutex);
                    while (mainWantsToEnter)
                        cv.wait(lk);
                    while (server.receive(packet, it) == sf::Socket::Status::Done)
                    {
                        packet >> upEvent;
                        upEventsRespondableBuffer.emplace_back(upEvent, it);
                        if (mainWantsToEnter)
                            break;
                    }
                }
            }
        });
    }

    std::cout << std::flush;
    std::cerr << std::flush;

    upEvents.emplace_back(UpEvent::Type::Ping);
    pingClock.restart();

    while (window.isOpen() || serverSide)
    {
        now = std::chrono::high_resolution_clock::now();
        delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - last).count();
        last = now;
        fps = (9.f * fps + 1.f / delta) / 10.f;
        ticks++;

        //client
        while (!serverSide && window.pollEvent(event))
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

        //Client
        if (!serverSide)
        {
            if (console.isTextEntered())
            {
                std::wstring command = console.get();
                if (command.find(L"join-team") == 0 || command.find(L"team-stats") == 0)
                    upEvents.emplace_back(UpEvent::Type::Command, command + L" " + std::to_wstring(Object::thisPlayerId));
                else
                    upEvents.emplace_back(UpEvent::Type::Command, command);
            }

            if (Object::thisPlayerId != -1)
                upEvents.emplace_back(UpEvent::Type::AimCoords, Object::thisPlayerId, Vec2f::asVec2f(window.mapPixelToCoords(sf::Mouse::getPosition(window))));

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
                    ping = (9.f * ping + pingClock.getElapsedTime().asSeconds()) / 10.f;
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
                    polygon.states.transform.translate(polygon.linearVelocity * delta).rotate(polygon.angularVelocity * delta, polygon.position);

                for (auto &player : downEventDirectDraw.players)
                    player.position += player.linearVelocity * delta;
            }

            window.clear(sf::Color::Black);

            for (const auto &player : downEventDirectDraw.players)
            {
                if (player.id == Object::thisPlayerId)
                {
                    window.setView({player.position, window.getView().getSize()});
                    cursor.setState(player.reload, player.aimState, player.hp, player.maxHp);
                    scale = {window.getView().getSize().x / (float)window.getSize().x, window.getView().getSize().y / (float)window.getSize().y};
                    alive = 100;
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
                //window.setView({{0.f, 0.f}, window.getView().getSize()});
                cursor.setState();
                scale = {window.getView().getSize().x / (float)window.getSize().x, window.getView().getSize().y / (float)window.getSize().y};
            }
            else
                alive--;

            for (const auto &polygon : downEventDirectDraw.polygons)
                window.draw(polygon.vertices, polygon.states);

            particleSystem.update(delta);

            text.setString(L"Fps:  "s + std::to_wstring((int)std::roundf(fps)) + L"\nPing: "s + std::to_wstring((int)std::roundf(ping * 1000.f / 2.f)));
            text.setScale(scale);
            text.setPosition(window.mapPixelToCoords({(int)window.getSize().x - 100, 18}));

            window.draw(text);
            window.draw(grid);
            window.draw(cursor);
            window.draw(particleSystem);
            window.draw(console);
            window.display();
        }
        //...

        //Server side
        if (serverSide)
        {
            {
                mainWantsToEnter = true;
                std::lock_guard<std::mutex> lk(serverMutex);
                std::swap(upEventsRespondable, upEventsRespondableBuffer);
                mainWantsToEnter = false;
            }
            cv.notify_one();

            for (auto &el : upEventsRespondable)
            {
                UpEvent &upEvent = el.first;
                if (Object::objects.count(upEvent.targetId) == 0 && upEvent.type != UpEvent::Type::Command && upEvent.type != UpEvent::Type::Invalid && upEvent.type != UpEvent::Type::Ping)
                    continue;
                upEventCounter++;

                switch (upEvent.type)
                {
                case UpEvent::Type::Forward:
                    dynamic_cast<Spaceship &>(*Object::objects[upEvent.targetId]).forward = upEvent.state;
                    break;
                case UpEvent::Type::Left:
                    dynamic_cast<Spaceship &>(*Object::objects[upEvent.targetId]).left = upEvent.state;
                    break;
                case UpEvent::Type::Right:
                    dynamic_cast<Spaceship &>(*Object::objects[upEvent.targetId]).right = upEvent.state;
                    break;
                case UpEvent::Type::Shoot:
                    dynamic_cast<Spaceship &>(*Object::objects[upEvent.targetId]).shoot = upEvent.state;
                    break;
                case UpEvent::Type::AimCoords:
                    dynamic_cast<Spaceship &>(*Object::objects[upEvent.targetId]).aimCoords = upEvent.coords;
                    break;
                case UpEvent::Type::Command:
                    if (upEvent.command.find(L"message ") == 0)
                    {
                        DownEvent message(DownEvent::Type::Message);
                        message.message = upEvent.command.erase(0, 8) + L"\n";
                        downEvents.push_back(message);
                        break;
                    }
                    {
                        DownEvent response(DownEvent::Type::Response);
                        response.message = commandProcessor.call(upEvent.command);
                        packet.clear();
                        packet << response;
                    }
                    {
                        mainWantsToEnter = true;
                        std::lock_guard<std::mutex> lk(serverMutex);
                        server.respond(packet, el.second);
                        mainWantsToEnter = false;
                    }
                    cv.notify_one();
                    break;
                case UpEvent::Type::Ping:
                    packet.clear();
                    packet << DownEvent(DownEvent::Type::Pong);
                    {
                        mainWantsToEnter = true;
                        std::lock_guard<std::mutex> lk(serverMutex);
                        server.respondUdp(packet, el.second);
                        mainWantsToEnter = false;
                    }
                    cv.notify_one();
                    break;
                }
            }
            upEventsRespondable.clear();

            commandProcessor.processJobs();
            Object::processAll();
            Object::world.Step(delta, velocityIterations, positionIterations, executor);

            lastActiveCounter = 0;

            if (server.getProtocol() == Server::protocol::TCP)
            {
                if (performanceLevelId != PerformanceLevels::Id::Low)
                {
                    if (clock_f.getElapsedTime().asSeconds() >= 0.04f)
                        clock_f.restart(), lastActiveCounter = server.getActiveCounter();
                }
                else
                    if (clock_f.getElapsedTime().asSeconds() >= 0.033f)
                        clock_f.restart(), lastActiveCounter = server.getActiveCounter();
            }
            else
                if (clock_f.getElapsedTime().asSeconds() >= 0.025f)
                    clock_f.restart(), lastActiveCounter = server.getActiveCounter();
            
            if (lastActiveCounter > 0)
            {
                renderSerializer.clear();
                for (const auto &object : Object::objects)
                    renderSerializer.draw(*(object.second));
                packet.clear();
                packet << renderSerializer.getDownEvent();
            }

            {
                mainWantsToEnter = true;
                std::lock_guard<std::mutex> lk(serverMutex);
                if (lastActiveCounter > 0)
                    server.sendToActive(packet);
                for (const auto &downEvent : downEvents)
                {
                    packet.clear();
                    packet << downEvent;
                    server.send(packet);
                }
                mainWantsToEnter = false;
            }
            cv.notify_one();
            downEvents.clear();
        }
        //...

        if (clock1.getElapsedTime().asSeconds() >= 1.f)
        {
            if (serverSide)
            {
                fps = float(ticks) / clock1.getElapsedTime().asSeconds();
                std::cout << upEventCounter << " events received.\n";
                std::cout << int(std::roundf(fps)) << " server frames per second.\n";
                if (server.getProtocol() == Server::protocol::TCP)
                    std::cout << "TCP is being used.\n";
                else
                    std::cout << "UDP is being used.\n";
                upEventCounter = 0;
                if (fps >= 100.f)
                {
                    if (performanceLevelId != PerformanceLevels::Id::High)
                    {
                        std::cout << "High perforance level.\n";
                        PerformanceLevels::set<PerformanceLevels::High>(performanceLevelId, velocityIterations, positionIterations, Bullet::minimumBulletVelocity);
                    }
                }
                else if (fps >= 50.f)
                {
                    if (performanceLevelId != PerformanceLevels::Id::Normal)
                    {
                        std::cout << "Normal perforance level.\n";
                        PerformanceLevels::set<PerformanceLevels::Normal>(performanceLevelId, velocityIterations, positionIterations, Bullet::minimumBulletVelocity);
                    }
                }
                else
                {
                    if (performanceLevelId != PerformanceLevels::Id::Low)
                    {
                        std::cout << "Low perforance level.\n";
                        PerformanceLevels::set<PerformanceLevels::Low>(performanceLevelId, velocityIterations, positionIterations, Bullet::minimumBulletVelocity);
                    }
                }
            }
            else
            {
                if (pingClock.getElapsedTime().asSeconds() > 1.f)
                {
                    pingClock.restart();
                    upEvents.emplace_back(UpEvent::Type::Ping);
                }
            }

            clock1.restart();
            ticks = 0;
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
