#ifndef RANKEDBATTLE_H_
#define RANKEDBATTLE_H_

#include <string>
#include "CommandProcessor.h"
#include "Event.h"
#include "Object.h"

template <class S>
class RankedBattle
{
public:
    RankedBattle(CommandProcessor &newCommandProcessor, std::queue<DownEvent> &newDownEvents, const S &newSecret)
        : commandProcessor(newCommandProcessor), downEvents(newDownEvents), secret(newSecret) {}

    Object::ObjectId start(const std::wstring shipType, const std::wstring pilotName)
    {

        Object::destroyAll();
        std::size_t n = 60;
        while (n--)
            Rock::create();

        Rock::create({{-width, pos - width}, {width, pos + width}, {-width, -pos + width}, {width, -pos - width}}, pos, 0.f);
        Rock::create({{-width, pos + width}, {width, pos - width}, {-width, -pos - width}, {width, -pos + width}}, -pos, 0.f);
        Rock::create({{pos - width, -width}, {pos + width, width}, {-pos + width, -width}, {-pos - width, width}}, 0.f, pos);
        Rock::create({{pos + width, -width}, {pos - width, width}, {-pos - width, -width}, {-pos + width, width}}, 0.f, -pos);

        Object::ObjectId id = Spaceship::create(CommandProcessor::converter.to_bytes(shipType), pilotName)->getId();

        if (httpFuture.valid())
            httpFuture.get();
        httpFuture = std::async(std::launch::async, [this, pilotName]() {
            sf::Http http("http://starship-battle.herokuapp.com/");
            //sf::Http http("http://127.0.0.1/", 3000);
            sf::Http::Request request;
            sf::Http::Response response;

            request.setMethod(sf::Http::Request::Post);
            request.setUri("api/results/start");
            request.setHttpVersion(1, 1);
            request.setField("Content-Type", "application/json");
            if (isDebuggerAttached())
            {
                std::cerr << "Debugger is attached." << std::endl;
                return;
            }
            request.setBody((json{
                                 {"pilotName", CommandProcessor::converter.to_bytes(pilotName)},
                                 {"secret", deobfuscate(secret)}})
                                .dump());
            response = http.sendRequest(request);

            if (response.getStatus() != sf::Http::Response::Created)
            {
                std::cerr << "Could not save score to server." << std::endl;
                std::cerr << "Status: " << response.getStatus() << std::endl;
                std::cerr << "Content-Type header: " << response.getField("Content-Type") << std::endl;
                std::cerr << "Body: " << response.getBody() << std::endl;
            }
        });

        begin = std::chrono::steady_clock::now();
        helper = std::chrono::steady_clock::now() - std::chrono::seconds(50);

        commandProcessor.job([this, id, shipType, pilotName]() {
            if (Object::objects.count(id) == 1)
            {
                now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::duration<float>>(now - helper).count() >= 60.f)
                {
                    helper = std::chrono::steady_clock::now();
                    downEvents.emplace(DownEvent::Type::Response);
                    downEvents.back().message = L"print Warning! Spawning new enemies!\n"s;
                    commandProcessor.call(L"create-bots"s);
                    Bot::allTarget(id);
                }
            }
            else
            {
                downEvents.emplace(DownEvent::Type::Response);
                downEvents.back().message = L"print Ranked battle ended.\n"s +
                                            L"You survived for: "s +
                                            std::to_wstring(int(std::chrono::duration_cast<std::chrono::duration<float>>(now - begin).count())) +
                                            L" seconds.\n"s;

                if (httpFuture.valid())
                    httpFuture.get();
                httpFuture = std::async(std::launch::async, [this, pilotName, shipType]() {
                    sf::Http http("http://starship-battle.herokuapp.com/");
                    //sf::Http http("http://127.0.0.1/", 3000);
                    sf::Http::Request request;
                    sf::Http::Response response;

                    request.setMethod(sf::Http::Request::Post);
                    request.setUri("api/results/end");
                    request.setHttpVersion(1, 1);
                    request.setField("Content-Type", "application/json");
                    if (isDebuggerAttached())
                    {
                        std::cerr << "Debugger is attached." << std::endl;
                        return;
                    }
                    request.setBody((json{
                                         {"pilotName", CommandProcessor::converter.to_bytes(pilotName)},
                                         {"shipType", CommandProcessor::converter.to_bytes(shipType)},
                                         {"secret", deobfuscate(secret)},
                                         {"hash", resourceManager::getJSON("hash")}})
                                        .dump());
                    response = http.sendRequest(request);

                    if (response.getStatus() != sf::Http::Response::Created)
                    {
                        std::cerr << "Could not save score to server." << std::endl;
                        std::cerr << "Status: " << response.getStatus() << std::endl;
                        std::cerr << "Content-Type header: " << response.getField("Content-Type") << std::endl;
                        std::cerr << "Body: " << response.getBody() << std::endl;
                    }
                });

                return L""s;
            }
            return L"next"s;
        });

        return id;
    }

private:
    CommandProcessor &commandProcessor;
    std::queue<DownEvent> &downEvents;
    const S &secret;
    std::chrono::steady_clock::time_point begin;
    std::chrono::steady_clock::time_point helper;
    std::chrono::steady_clock::time_point now;
    std::future<void> httpFuture;

    static constexpr float width = 20.f; // Half of width
    static constexpr float pos = 400.f;
};

#endif /* !RANKEDBATTLE_H_ */
