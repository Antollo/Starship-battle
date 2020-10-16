#ifndef NETWORK_H_
#define NETWORK_H_

#include <vector>
#include <utility>
#include <memory>
#include <iostream>
#include <atomic>
#include <unordered_map>
#include <SFML/Network.hpp>

namespace std
{
    template <>
    struct hash<sf::IpAddress>
    {
        size_t operator()(const sf::IpAddress &ip) const
        {
            return hash<unsigned int>()(ip.toInteger());
        }
    };
}; // namespace std

class Client
{
public:
    Client(sf::TcpSocket &newSocket, sf::UdpSocket &newUdpSocket) : socket(newSocket), udpSocket(newUdpSocket)
    {
        selector.add(socket);
        selector.add(udpSocket);
    }

    bool wait(sf::Time timeout = sf::microseconds(1))
    {
        return selector.wait(timeout);
    }

    sf::Socket::Status receive(sf::Packet &packet)
    {
        if (!wait())
            return sf::Socket::NotReady;
            
        if (selector.isReady(udpSocket))
        {
            sf::IpAddress remoteAddress;
            unsigned short remotePort;
            return udpSocket.receive(packet, remoteAddress, remotePort);
        }
        if (selector.isReady(socket))
            return socket.receive(packet);

        return sf::Socket::Error;
    }

    sf::Socket::Status send(sf::Packet &packet)
    {
        return socket.send(packet);
    }

    sf::Socket::Status sendUdp(sf::Packet &packet)
    {
        return udpSocket.send(packet, socket.getRemoteAddress(), socket.getRemotePort());
    }

private:
    sf::TcpSocket &socket;
    sf::UdpSocket &udpSocket;
    sf::SocketSelector selector;
};

class Server
{
public:
    using iterator = std::pair<sf::TcpSocket *, sf::IpAddress>;
    enum class protocol
    {
        UDP,
        TCP
    };

    Server(sf::TcpListener &newListener, sf::UdpSocket &newUdpSocket) : listener(newListener), udpSocket(newUdpSocket), activeCounter(0)
    {
        selector.add(listener);
        selector.add(udpSocket);
    }

    bool wait(sf::Time timeout = sf::microseconds(1))
    {
        return selector.wait(timeout);
    }

    sf::Socket::Status receive(sf::Packet &packet, iterator &remote)
    {
        if(!wait())
            return sf::Socket::NotReady;

        if (selector.isReady(listener))
        {
            clients.push_back(std::make_unique<sf::TcpSocket>());
            if (listener.accept(*clients.back()) == sf::Socket::Done)
            {
                selector.add(*clients.back());
                std::cout << "New client connected.\n";
            }
            else
            {
                clients.pop_back();
            }
        }

        if (selector.isReady(udpSocket))
        {
            unsigned short port;
            sf::Socket::Status status = udpSocket.receive(packet, remote.second, port);
            if (active[remote.second] == false)
            {
                activeCounter++;
                active[remote.second] = true;
            }
            return status;
        }

        for (auto it = clients.begin(); it != clients.end();)
        {
            auto jt = it++;
            sf::TcpSocket &current = **jt;
            if (selector.isReady(current))
            {
                remote.first = &current;
                remote.second = current.getRemoteAddress();
                if (active[remote.second] == false)
                {
                    activeCounter++;
                    active[remote.second] = true;
                }
                sf::Socket::Status status = current.receive(packet);
                if (status == sf::Socket::Disconnected)
                {
                    std::cout << "Client disconnected.\n";
                    activeCounter--;
                    active.erase(remote.second);
                    selector.remove(current);
                    current.disconnect();
                    clients.erase(jt);
                }
                return status;
            }
        }
        return sf::Socket::Status::Error;
    }

    sf::Socket::Status respond(sf::Packet &packet, iterator &remote)
    {
        return remote.first->send(packet);
    }

    sf::Socket::Status respondUdp(sf::Packet &packet, iterator &remote)
    {
        return udpSocket.send(packet, remote.second, udpSocket.getLocalPort() + 1);
    }

    sf::Socket::Status send(sf::Packet &packet)
    {
        for (auto &client : clients)
        {
            client->send(packet);
        }
        return sf::Socket::Status::Done;
    }

    sf::Socket::Status sendToActive(sf::Packet &packet)
    {
        if (packet.getDataSize() < sf::UdpSocket::MaxDatagramSize)
            return sendToActiveUdp(packet);
        else
            return sendToActiveTcp(packet);
    }

    int getActiveCounter() const { return activeCounter; }
    protocol getProtocol() const
    {
        return static_cast<protocol>(static_cast<int>(p));
    }

private:
    sf::Socket::Status sendToActiveUdp(sf::Packet &packet)
    {
        p = static_cast<int>(protocol::UDP);
        for (auto &pair : active)
        {
            if (pair.second == true)
            {
                pair.second = false;
                activeCounter--;
                udpSocket.send(packet, pair.first, udpSocket.getLocalPort() + 1);
            }
        }
        return sf::Socket::Status::Done;
    }

    sf::Socket::Status sendToActiveTcp(sf::Packet &packet)
    {
        p = static_cast<int>(protocol::TCP);
        sf::IpAddress address;
        for (auto &client : clients)
        {
            address = client->getRemoteAddress();
            if (active[address] == true)
            {
                active[address] = false;
                activeCounter--;
                client->send(packet);
            }
        }
        return sf::Socket::Status::Done;
    }
    std::atomic<int> activeCounter, p;
    sf::TcpListener &listener;
    sf::UdpSocket &udpSocket;
    std::vector<std::unique_ptr<sf::TcpSocket>> clients;
    std::unordered_map<sf::IpAddress, bool> active;
    sf::SocketSelector selector;
};

#endif /* !NETWORK_H_ */
