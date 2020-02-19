#ifndef NETWORK_H_
#define NETWORK_H_

#include <vector>
#include <utility>
#include <memory>
#include <iostream>
#include <SFML/Network.hpp>

class Client
{
public:
    Client(sf::TcpSocket &newSocket) : socket(newSocket)
    {
        selector.add(socket);
    }

    sf::Socket::Status receive(sf::Packet &packet, sf::Time timeout = sf::microseconds(1))
    {
        if (selector.wait(timeout))
            if (selector.isReady(socket))
                return socket.receive(packet);
        return sf::Socket::NotReady;
    }

    sf::Socket::Status send(sf::Packet &packet)
    {
        return socket.send(packet);
    }

private:
    sf::TcpSocket &socket;
    sf::SocketSelector selector;
};

class Server
{
public:
    using iterator = std::vector<std::pair<std::unique_ptr<sf::TcpSocket>, bool>>::iterator;

    Server(sf::TcpListener &newListener) : listener(newListener)
    {
        selector.add(listener);
    }

    sf::Socket::Status receive(sf::Packet &packet, iterator& last, sf::Time timeout = sf::microseconds(1))
    {
        if (selector.wait(timeout))
        {
            if (selector.isReady(listener))
            {
                clients.push_back({std::make_unique<sf::TcpSocket>(), false});
                if (listener.accept(*clients.back().first) == sf::Socket::Done)
                {
                    selector.add(*clients.back().first);
                    std::cout << "New client connected.\n";
                }
                else
                {
                    clients.pop_back();
                }
            }

            for (auto it = clients.begin(); it != clients.end(); it++)
            {
                if (selector.isReady(*it->first))
                {
                    last = it;
                    it->second = true;
                    sf::Socket::Status status = it->first->receive(packet);
                    if (status == sf::Socket::Disconnected)
                    {
                        std::cout << "Client disconnected.\n";
                        selector.remove(*it->first);
                        it->first->disconnect();
                        clients.erase(it);
                    }
                    return status;
                }
            }
        }
        return sf::Socket::Status::Error;
    }

    sf::Socket::Status respond(sf::Packet &packet, iterator& last)
    {
        return last->first->send(packet);
    }

    sf::Socket::Status sendToActive(sf::Packet &packet)
    {
        for (auto &pair : clients)
        {
            if (pair.second)
            {
                pair.second = false;
                pair.first->send(packet);
            }
        }
        return sf::Socket::Status::Done;
    }

    sf::Socket::Status send(sf::Packet &packet)
    {
        for (auto &pair : clients)
        {
            pair.first->send(packet);
        }
        return sf::Socket::Status::Done;
    }

private:
    sf::TcpListener &listener;
    std::vector<std::pair<std::unique_ptr<sf::TcpSocket>, bool>> clients;
    sf::SocketSelector selector;
};

#endif /* !NETWORK_H_ */
