#ifndef SERVER_H
#define SERVER_H

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <chrono>
#include <string>
#include <future>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <SFML/Graphics.hpp>
#include <SFML/Config.hpp>
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

class GameServer
{
private:
    Args args;

public:
    GameServer(Args &&myArgs) : args(myArgs) {}
    int operator()()
    {
        int port = std::stoi(args["port"]);
        sf::UdpSocket udpSocket;
        std::chrono::high_resolution_clock::time_point now, last = std::chrono::high_resolution_clock::now();
        float delta = 0.f;
        sf::Clock clock1, clock_f;
        constexpr int velocityIterations = 10, positionIterations = 4;
        constexpr float respondToActiveInterval = 1.f / 150.f;
        int ticks = 0;
        float fps = 100.f;
        std::atomic<bool> running = true, mainWantsToEnter = false;
        std::condition_variable cv;
        std::future<void> receivingThread;
        Console console; //This thing displaying text ui
        console
            << L"Spaceship commander command prompt\n"
            << L"Server, "
            << L"version from " << CommandProcessor::converter.from_bytes(__DATE__) << L" " << CommandProcessor::converter.from_bytes(__TIME__) << L"\n"
            << L"Use 'help' command to learn the basics.\n\n";

        //Server
        int upEventCounter = 0;
        CommandProcessor commandProcessor; //For scripting in console (commands are run on server side)
        sf::TcpListener listener;          //Server
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

        Server server(listener, udpSocket);
        RenderSerializer renderSerializer; //"Display" to Packet
        UpEvent upEvent;
        std::vector<DownEvent> downEvents;
        std::vector<std::pair<UpEvent, Server::iterator>> upEventsRespondable, upEventsRespondableBuffer;
        std::mutex serverMutex;
        ContactListener contactListener(downEvents);
        Object::world.SetContactListener(&contactListener);
        b2ThreadPoolTaskExecutor executor;
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

        Map::setMap(Map::create());

        receivingThread = std::async(std::launch::async, [&server, &running, &serverMutex, &upEventsRespondableBuffer, &cv, &mainWantsToEnter]() {
            sf::Packet packet;
            Server::iterator it;
            UpEvent upEvent;
            while (running)
            {
                if (server.wait())
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

        std::cout << std::flush;
        std::cerr << std::flush;

        while (true)
        {
            //std::this_thread::sleep_for(std::chrono::milliseconds(20));
            now = std::chrono::high_resolution_clock::now();
            delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - last).count();
            last = now;
            fps = (9.f * fps + 1.f / delta) / 10.f;
            ticks++;

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
                Object::ObjectsContainer::iterator it;
                upEventCounter++;

                switch (upEvent.type)
                {
                case UpEvent::Type::Forward:
                    if ((it = Object::objects.find(upEvent.targetId)) != Object::objects.end())
                        dynamic_cast<Spaceship &>(*it->second).forward = upEvent.state;
                    break;
                case UpEvent::Type::Left:
                    if ((it = Object::objects.find(upEvent.targetId)) != Object::objects.end())
                        dynamic_cast<Spaceship &>(*it->second).left = upEvent.state;
                    break;
                case UpEvent::Type::Right:
                    if ((it = Object::objects.find(upEvent.targetId)) != Object::objects.end())
                        dynamic_cast<Spaceship &>(*it->second).right = upEvent.state;
                    break;
                case UpEvent::Type::Shoot:
                    if ((it = Object::objects.find(upEvent.targetId)) != Object::objects.end())
                        dynamic_cast<Spaceship &>(*it->second).shoot = upEvent.state;
                    break;
                case UpEvent::Type::AimCoords:
                    if ((it = Object::objects.find(upEvent.targetId)) != Object::objects.end())
                        dynamic_cast<Spaceship &>(*it->second).aimCoords = upEvent.coords;
                    break;
                case UpEvent::Type::Command:
                    if (upEvent.command.find(L"message ") == 0)
                    {
                        DownEvent message(DownEvent::Type::Message);
                        message.message = upEvent.command.erase(0, 8) + L"\n";
                        downEvents.push_back(message);
                    }
                    else if (upEvent.command.find(L"m ") == 0)
                    {
                        DownEvent message(DownEvent::Type::Message);
                        message.message = upEvent.command.erase(0, 2) + L"\n";
                        downEvents.push_back(message);
                    }
                    else
                    {

                        DownEvent response(DownEvent::Type::Response);
                        response.message = commandProcessor.call(upEvent.command);
                        packet.clear();
                        packet << response;
                        {
                            mainWantsToEnter = true;
                            std::lock_guard<std::mutex> lk(serverMutex);
                            server.respond(packet, el.second);
                            mainWantsToEnter = false;
                        }
                        cv.notify_one();
                    }
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
                case UpEvent::Type::MissingShape:
                    {
                        ServerShape *shape = ServerShape::getShape(upEvent.targetId);
                        if(shape)
                        {
                            std::wcout<<"Sent " << upEvent.targetId << std::endl;

                            packet.clear();
                            packet << DownEvent(DownEvent::Type::MissingShape, shape, upEvent.targetId);
                            {
                                mainWantsToEnter = true;
                                std::lock_guard<std::mutex> lk(serverMutex);
                                server.respondUdp(packet, el.second);
                                mainWantsToEnter = false;
                            }
                            cv.notify_one();
                        }
                        else
                            std::wcout<<"Shape " << upEvent.targetId << " not found" << std::endl;
                        
                    }
                    break;
                }
            }
            upEventsRespondable.clear();

            commandProcessor.processJobs();
            Object::processAll(delta);
            Object::world.Step(delta, velocityIterations, positionIterations, executor);

            lastActiveCounter = 0;

            if (clock_f.getElapsedTime().asSeconds() >= respondToActiveInterval)
            {
                clock_f.restart();
                lastActiveCounter = server.getActiveCounter();
            }

            if (lastActiveCounter > 0)
            {
                renderSerializer.clear();
                for (const auto &object : Object::objects)
                    renderSerializer.draw(*(object.second));
                packet.clear();
                packet << renderSerializer.getDownEvent();
            }

            if (lastActiveCounter > 0 || !downEvents.empty())
            {
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

            if (clock1.getElapsedTime().asSeconds() >= 1.f)
            {
                fps = float(ticks) / clock1.getElapsedTime().asSeconds();
                std::cout << upEventCounter << " events received.\n";
                std::cout << int(std::roundf(fps)) << " server frames per second.\n";
                if (server.getProtocol() == Server::protocol::TCP)
                    std::cout << "TCP is being used.\n";
                else
                    std::cout << "UDP is being used.\n";
                upEventCounter = 0;
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
};

#endif /* SERVER_H */
