#ifndef NETWORK_H_
#define NETWORK_H_

#include <vector>
#include <memory>
#include <iostream>
#include <SFML/Network.hpp>

class Client 
{
public:
    Client(sf::TcpSocket& newSocket) : socket(newSocket)
    {
        selector.add(socket);
    }

    sf::Socket::Status receive(sf::Packet& packet, sf::Time timeout = sf::milliseconds(1))
    {
        if (selector.wait(timeout))
            if (selector.isReady(socket))
                return socket.receive(packet);
        return sf::Socket::Error;
    }
    
    sf::Socket::Status send(sf::Packet& packet)
    {
        return socket.send(packet);
    }
private:
    sf::TcpSocket& socket;
    sf::SocketSelector selector;
};

class Server 
{
public:
    Server(sf::TcpListener& newListener) : listener(newListener)
    {
        selector.add(listener);
    }

    sf::Socket::Status receive(sf::Packet& packet, sf::Time timeout = sf::milliseconds(1)) 
    {
        if (selector.wait(timeout))
        {
            if (selector.isReady(listener))
            {
                clients.push_back(std::make_unique<sf::TcpSocket>());
                if (listener.accept(*clients.back()) == sf::Socket::Done)
                {
                    selector.add(*clients.back());
                    std::cout << "New client connected\n";
                }
                else
                {
                    clients.pop_back();
                }
            }
    
            for (auto it = clients.begin(); it != clients.end(); it++)
            {
                if (selector.isReady(**it))
                {
                    lastClient = it;
                    return (*it)->receive(packet);
                }
            }
        }
        return sf::Socket::Status::Error;
    }

    sf::Socket::Status respond(sf::Packet& packet)
    {
        return (*lastClient)->send(packet);
    }

    sf::Socket::Status send(sf::Packet& packet)
    {
        for (auto& ptr : clients)
        {
			ptr->send(packet);
        }
        return sf::Socket::Status::Done;
    }

private:
    sf::TcpListener& listener;
    std::vector<std::unique_ptr<sf::TcpSocket>> clients;
    std::vector<std::unique_ptr<sf::TcpSocket>>::iterator lastClient;
    sf::SocketSelector selector;
};

#endif /* !NETWORK_H_ */
