#include "Client.h"
#include "Server.h"

#define SFML_DEFINE_DISCRETE_GPU_PREFERENCE

int main(int argc, const char *argv[])
{
    static_assert(sizeof(b2Vec2) == sizeof(Vec2f));
    static_assert(sizeof(sf::Vector2f) == sizeof(Vec2f));

    Args args(argc, argv);
    bool serverSide = args["server"].size();
    if (serverSide)
    {
        GameServer s(std::move(args));
        return s();
    }
    else
    {
        GameClient c(std::move(args));
        return c();
    }
}
