#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <chrono>
#include <queue>
#include <string>
#include <SFML/Graphics.hpp>
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


int main(int argc, char *argv[])
{
    //for (size_t i = 0; i < argc; i++)
    //    std::cout<<argv[i]<<std::endl;

    static_assert(sizeof(b2Vec2) == sizeof(Vec2f));
    static_assert(sizeof(sf::Vector2f) == sizeof(Vec2f));
    if (argc < 2)
    {
        std::cerr << "Too little arguments provided.\n";
        return 0;
    }
    std::string arg = argv[1];
    int tick = 0, upEventCounter = 0;
    bool serverSide = (arg == "server");
    if (!serverSide && argc < 3)
    {
        std::cerr << "Too little arguments provided.\n";
        return 0;
    }

    Grid grid;
    Cursor cursor;
    ParticleSystem particleSystem;
    Console console; //This thing displaying text ui
    CommandProcessor commandProcessor; //For scripting in console (commands are run on server side)

    sf::TcpListener listener; //Server
    if (serverSide)
        listener.listen(1717);
    Server server(listener);
    RenderSerializer renderSerializer; //"Display" to Packet
    UpEvent upEvent;
    std::queue<DownEvent> downEvents;


    sf::TcpSocket socket; //Client
    sf::Text text;    
	if (!serverSide)
    {
        int port = std::stoi(std::string(argv[2]));
        if (socket.connect(arg, port) != sf::Socket::Status::Done)
        {
            if (socket.connect(arg, port) != sf::Socket::Status::Done)
            {
                std::cerr << "Could not connect with " << arg << ":" << port << ".\n";
                std::cerr << "Please try again later." << std::flush;
                return 1;
            }
        }
        text.setFont(resourceManager::getFont("UbuntuMono.ttf"));
        text.setCharacterSize(18);
        text.setFillColor(sf::Color::White);
    }
    Client client(socket);
    DownEvent downEvent, downEventDirectDraw;
    bool directDrew = false, alive = false, rightMouseButtonDown = false;
    decltype(Object::objects)::iterator i, j;
    float m = 1.f;
    Vec2f scale;
    std::queue<UpEvent> upEvents;

    sf::RenderWindow window; //Graphic window
    float fps = 0.f;
    //sf::RenderTexture renderTexture;
    //sf::Shader shader;
    //sf::Sprite processed;
    //sf::Texture processedTexture;


    commandProcessor.bind(L"create", [](std::wstring shipType, std::wstring pilotName) {
        Object::ObjectId id = Spaceship::create(CommandProcessor::converter.to_bytes(shipType), pilotName)->getId();
        return L"setThisPlayerId "s + std::to_wstring(id) + L" print Spaceship is ready.\n"s; 
    });
    commandProcessor.bind(L"create-bot", [](std::wstring shipType) {
        Bot::create(CommandProcessor::converter.to_bytes(shipType));
        return L"print Bot is ready.\n"s;
    });
    commandProcessor.bind(L"create-bots", []() {
        std::ifstream file("config.json");
        if (!file.good()) throw std::runtime_error("config.json not found.");
        json jsonObject = json::parse(file);
        std::vector<std::string> spaceships = jsonObject["spaceships"].get<std::vector<std::string>>();
        std::size_t i = 0;
        for (const auto& name : spaceships)
        {
            i = 2;
            while (i--)
                Bot::create(name);
        }
        return L"print Bots are ready.\n"s;
    });
    commandProcessor.bind(L"credits", []() {
        return L"print "
        L"SFML          - www.sfml-dev.org\n"
        L"Box2D         - box2d.org\n"
        L"nlohmann/json - github.com/nlohmann/json\n"
        L"Electron      - electronjs.org\n"
        L"Ubuntu Mono   - fonts.google.com/specimen/Ubuntu+Mono\n"
        L"freesound     - freesound.org/people/spaciecat/sounds/456779\n"
        L"                freesound.org/people/Diboz/sounds/213925\n"
        L"                freesound.org/people/ryansnook/sounds/110111\n"
        L"                freesound.org/people/debsound/sounds/437602\n"s;

    });
    commandProcessor.bind(L"delete", []() {
        return L"print Monika.chr deleted successfully.\n"s;
    });
    commandProcessor.bind(L"list-spaceships", []() {
        std::wstring res = L"print "s;
        std::ifstream file("config.json");
        if (!file.good()) throw std::runtime_error("config.json not found.");
        json jsonObject = json::parse(file);
        std::vector<std::string> spaceships = jsonObject["spaceships"].get<std::vector<std::string>>();
        for (const auto& name : spaceships)
        {
            res += CommandProcessor::converter.from_bytes(name.c_str()) + L"\n"s;
        }
        return res;
    });
    commandProcessor.bind(L"beep", []() {
        resourceManager::playSound("glitch.wav");
        return L"print Server beeped.\n"s;
    });
    commandProcessor.bind(L"help", [](std::wstring arg) {
        return
        L"print \n"
        L"----------------------------------\n"
        L"Spaceship commander command prompt\n"
        L"Remote server, version from " + CommandProcessor::converter.from_bytes(__DATE__) + L" " + CommandProcessor::converter.from_bytes(__TIME__) + L"\n\n"
        L"Use W key to accelerate\n"
        L"Use A and D keys to rotate\n"
        L"Use mouse right button to aim\n"
        L"Use mouse left button to shoot\n"
        L"Use mouse wheel to scale the view\n"
        L"Use tilde key to switch console input\n"
        L"Use up, down, and tab keys as in normal console\n\n"
        L"List of commands:\n"
        L"    create [spaceship type] [pilot name] (aliased as 'c')\n"
        L"    create-bot [spaceship type]          (aliased as 'cb')\n"
        L"    create-bots                          (aliased as 'cbs')\n"
        L"    list-spaceships                      (aliased as 'ls')\n"
        L"    help                                 (aliased as 'h')\n"
        L"    delete                              \n"
        L"    help                                \n"
        L"    credits                             \n"
        L"    beep                                \n"s;
    });

    commandProcessor.alias(L"create", L"c");
    commandProcessor.alias(L"create-bot", L"cb");
    commandProcessor.alias(L"create-bots", L"cbs");
    commandProcessor.alias(L"list-spaceships", L"ls");
    commandProcessor.alias(L"help", L"h");

    // Job printing each letter of its arg in next server tick
    commandProcessor.bind(L"job-api-test", [&commandProcessor, &downEvents](std::wstring arg) {
        commandProcessor.job([&downEvents, arg]() {

            //Static persistant variables
            static std::wstring str = arg;
            
            // downEvents.emplace(DownEvent::Type::Response) == Write to all connected consoles

            if (str.empty())
            {
                //Job ends when its return is empty
                downEvents.emplace(DownEvent::Type::Response);
                downEvents.back().message = L"print Job ended.\n"s;
            }
            else
            {
                downEvents.emplace(DownEvent::Type::Response);
                downEvents.back().message = L"print "s + str.back() + L"\n"s;
                str.pop_back();
            }
            return str;
        });
        return L"print Job started.\n"s;
    });
    
    //"Welcome screen":
    console
    << L"----------------------------------\n"
    << L"Spaceship commander command prompt\n"
    << (serverSide ? L"Server, " : L"Client, ")
    << L"version from " << CommandProcessor::converter.from_bytes(__DATE__) << L" " << CommandProcessor::converter.from_bytes(__TIME__) << L"\n"
    << L"Use 'help' command to learn the basics.\n\n";
    
    //Server:
    //Create some rocks:
    if (serverSide)
    {
        std::size_t n = 60;
        while (n--) Rock::create();
    }
    
    //For physics:
    std::chrono::high_resolution_clock::time_point now, last = std::chrono::high_resolution_clock::now();
    float delta = 0.f;
    ContactListener contactListener(downEvents);
    Object::world.SetContactListener(&contactListener);
    sf::Event event;
    sf::Packet packet;

    if (!serverSide)
    {
        int antialiasingLevel = 16;
        std::ifstream file("config.json");
        if (file.good())
        {
            json jsonObject = json::parse(file);
            m = jsonObject["mouseScrollMultiplier"].get<float>();
            antialiasingLevel = jsonObject["antialiasingLevel"].get<int>();
        }
        
        resourceManager::playSound("glitch.wav");
        window.create(sf::VideoMode::getFullscreenModes().front(), "Starship battle", sf::Style::Fullscreen, sf::ContextSettings(0, 0, antialiasingLevel, 1, 1, 0, false));
        //renderTexture.create(window.getSize().x, window.getSize().y, sf::ContextSettings(0, 0, 16, 1, 1, 0, false));
        window.setVerticalSyncEnabled(true);
        window.setMouseCursorVisible(false);
        window.requestFocus();
        window.setView({{0.f, 0.f}, window.getView().getSize()});
        //processedTexture.create(window.getSize().x, window.getSize().y);
        //processed.setTexture(processedTexture);
        /*if (!shader.loadFromFile("crt-geom.vert", "crt-geom.frag"))
        {
            std::cerr << "Shader not loaded.\n";
        }*/
    }

    std::cout << std::flush;
    std::cerr << std::flush;

    while (window.isOpen() || serverSide)
    {
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
                if (event.mouseWheelScroll.delta*m < 0.f && view.getSize().x/(float)window.getSize().x + view.getSize().y/(float)window.getSize().y < 0.25f)
                    break;
                if (event.mouseWheelScroll.delta*m > 0.f && view.getSize().x/(float)window.getSize().x + view.getSize().y/(float)window.getSize().y > 32.f)
                    break;
                view.zoom((event.mouseWheelScroll.delta*m > 0.f) * 1.2f + (event.mouseWheelScroll.delta*m < 0) * 0.8f);
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
                            upEvents.emplace(UpEvent::Type::Forward, Object::thisPlayerId, ev);
                            break;
                        case sf::Keyboard::A:
                            upEvents.emplace(UpEvent::Type::Left, Object::thisPlayerId, ev);
                            break;
                        case sf::Keyboard::D:
                            upEvents.emplace(UpEvent::Type::Right, Object::thisPlayerId, ev);
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
                            rightMouseButtonDown = ev;
                            upEvents.emplace(UpEvent::Type::Aim, Object::thisPlayerId, ev);
                            break;
                        case sf::Mouse::Button::Left:
                            upEvents.emplace(UpEvent::Type::Shoot, Object::thisPlayerId, ev);
                            break;
                        /*default:
                            upEvents.emplace(UpEvent::Type::AimCoords, Object::thisPlayerId, Vec2f::asVec2f(window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y})));
                            break;*/
                    }
                }
                /*if (event.type == sf::Event::MouseMoved)
                    upEvents.emplace(UpEvent::Type::AimCoords, Object::thisPlayerId, Vec2f::asVec2f(window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y})));*/
            }
        }

        now = std::chrono::high_resolution_clock::now();
        delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - last).count();
        last = now;

        //Server side
        if (serverSide)
        {
            while (server.receive(packet) == sf::Socket::Status::Done)
            {
                packet >> upEvent;

                if (Object::objects.count(upEvent.targetId) == 0 && upEvent.type != UpEvent::Type::Command && upEvent.type != UpEvent::Type::Invalid)
                {
                    continue;
                }

                upEventCounter ++;

                switch (upEvent.type)
                {
                case UpEvent::Type::Forward:
                    dynamic_cast<Spaceship&>(*Object::objects[upEvent.targetId]).forward = upEvent.state;
                    break;
                case UpEvent::Type::Left:
                    dynamic_cast<Spaceship&>(*Object::objects[upEvent.targetId]).left = upEvent.state;
                    break;
                case UpEvent::Type::Right:
                    dynamic_cast<Spaceship&>(*Object::objects[upEvent.targetId]).right = upEvent.state;
                    break;
                case UpEvent::Type::Aim:
                    dynamic_cast<Spaceship&>(*Object::objects[upEvent.targetId]).aim = upEvent.state;
                    break;
                case UpEvent::Type::Shoot:
                    dynamic_cast<Spaceship&>(*Object::objects[upEvent.targetId]).shoot = upEvent.state;
                    break;
                case UpEvent::Type::AimCoords:
                    dynamic_cast<Spaceship&>(*Object::objects[upEvent.targetId]).aimCoords = upEvent.coords;
                    break;
                case UpEvent::Type::Command:
                    DownEvent response(DownEvent::Type::Response);
                    response.message = commandProcessor.call(upEvent.command);
                    packet.clear();
                    packet << response;
                    server.respond(packet);
                    break;
                }
            }

            commandProcessor.processJobs();

            //for (const auto& object : Object::objects)
            //    object.second->process();
            for (i = Object::objects.begin(); i != Object::objects.end();)
            {
                j = i++;
                j->second->process();
            }

            Object::world.Step(delta, 8, 3);

            renderSerializer.clear();
            for (const auto& object : Object::objects)
                renderSerializer.draw(*(object.second));
            packet.clear();
            packet << renderSerializer.getDownEvent();
            server.respondActive(packet);
            while (!downEvents.empty())
            {
                packet.clear();
                packet << downEvents.front();
                server.send(packet);
                downEvents.pop();
            }
        }
        //...

        //Client
        if (!serverSide)
        {
            if (console.isTextEntered())
                upEvents.emplace(UpEvent::Type::Command, console.get());
                //console << commandProcessor.call(console.get());

            if (rightMouseButtonDown)
                    upEvents.emplace(UpEvent::Type::AimCoords, Object::thisPlayerId, Vec2f::asVec2f(window.mapPixelToCoords(sf::Mouse::getPosition(window))));
                    
            if (upEvents.empty())
                upEvents.emplace();

            while (!upEvents.empty())
            {
                packet.clear();
                packet << upEvents.front();
                client.send(packet);
                upEvents.pop();
            }

            directDrew = false;

            while (client.receive(packet) == sf::Socket::Status::Done)
            {
                packet >> downEvent;
                switch (downEvent.type)
                {
                case DownEvent::Type::DirectDraw:
                    if (directDrew) break;
                    directDrew = true;
                    downEventDirectDraw = downEvent;
                    break;
                case DownEvent::Type::Collision:
                    if (downEvent.explosion)
                    {
                        resourceManager::playSound("explosion.wav");
                        particleSystem.impulse(downEvent.collision);
                    }
                    else
                        resourceManager::playSound("ricochet.ogg");
                    console << downEvent.message;
                    break;
                case DownEvent::Type::Message:
                    console << downEvent.message;
                    break;
                case DownEvent::Type::Response:
                    std::wstringstream wss;
                    std::wstring temp;
                    wss.str(downEvent.message);
                    while (wss >> temp)
                    {
                        if (temp == L"print"s)
                        {
                            //std::istream_iterator<std::wstring, wchar_t, std::char_traits<wchar_t>> it(wss), end;
                            //console << std::wstring(it, end);
                            wss.seekg((int)wss.tellg() + 1);
                            std::getline(wss, temp, (wchar_t)std::char_traits<wchar_t>::eof());
                            console << temp;
                        }
                        else if (temp == L"setThisPlayerId"s)
                            wss >> Object::thisPlayerId;
                    }
                    //console << downEvent.message;
                    break;
                }
            }

            window.clear(sf::Color::Black);

            alive = false;
            for (const auto& player : downEventDirectDraw.players)
            {
                if (player.id == Object::thisPlayerId)
                {
                    window.setView({player.coords, window.getView().getSize()});
                    cursor.setState(player.reload, player.hp, player.maxHp);
                    scale = {window.getView().getSize().x/(float)window.getSize().x, window.getView().getSize().y/(float)window.getSize().y};
                    alive = true;
                }
                text.setString(player.playerId + L"\nHp: "s + std::to_wstring(player.hp) + L"/"s + std::to_wstring(player.maxHp));
                text.setPosition(player.coords.x, player.coords.y + 500.f / std::max(std::sqrt(scale.y), 2.f));
                text.setScale(scale);
                window.draw(text);

            }
            if (!alive)
            {
                Object::thisPlayerId = -1;
                //window.setView({{0.f, 0.f}, window.getView().getSize()});
                cursor.setState();
                scale = {window.getView().getSize().x/(float)window.getSize().x, window.getView().getSize().y/(float)window.getSize().y};
            }
            
            for (const auto& polygon : downEventDirectDraw.polygons)
            {
                window.draw(polygon.vertices, polygon.states);
            }

            particleSystem.update(delta);

            fps = (2.f * fps + 1.f/delta)/3.f;
            text.setString(L"Fps: "s + std::to_wstring((int)std::roundf(fps)));
            text.setScale(scale);
            text.setPosition(window.mapPixelToCoords({(int) window.getSize().x - 80, 18}));
            window.draw(text);

            window.draw(grid);
            window.draw(cursor);
            window.draw(particleSystem);
            window.draw(console);
            window.display();
            
            //processedTexture.update(window);

            /*window.clear(sf::Color::Red);
            sf::View temp = window.getView();
            window.setView(sf::View(sf::FloatRect(0.f , 0.f, (float) window.getSize().x, (float) window.getSize().y)));
            sf::Vector2f pos(18.f, -18.f + (float) window.getView().getSize().y);
            processed.setPosition(pos);
            shader.setUniform("texture",sf::Shader::CurrentTexture);
            processed.setTexture(renderTexture.getTexture());
            window.draw(processed, &shader);
            window.setView(temp);
            window.display();*/
        }
        //...
        tick++;
        if (tick >= 10000)
        {
            tick = 0;
            if(serverSide) {
                std::cout << upEventCounter << " UpEvents received.\n";
                upEventCounter = 0;
            }
            std::cout << std::flush;
            std::cerr << std::flush;
        }
    }
	Object::objects.clear();

    return 0;
}
