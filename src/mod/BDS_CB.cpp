#include "mod/BDS_CB.h"
#include "mod/httplib.h"

#include <nlohmann/json.hpp>

#include <ll/api/service/Bedrock.h>
#include <ll/api/mod/RegisterHelper.h>

#include <ll/api/event/EventBus.h>
#include <ll/api/thread/ThreadPoolExecutor.h>
#include <ll/api/event/player/PlayerChatEvent.h>

#include <mc/deps/core/platform/BuildPlatform.h>
#include <mc/world/Level/level.h>
#include <mc/world/Level/dimension/Dimension.h>
#include <mc/world/actor/player/Player.h>

#include <unordered_map>

namespace bds_chatbot {

using json = nlohmann::json;
void getData(std::vector<std::string>& tags, json& ctx);
std::string askAI(const std::string& prompt);

BDS_CB& BDS_CB::getInstance() {
    static BDS_CB instance;
    return instance;
}

static std::vector<ll::event::ListenerPtr> gListeners;

bool BDS_CB::load() {
    // getSelf().getLogger().debug("Loading...");
    return true;
}

bool BDS_CB::enable() {
    auto& bus = ll::event::EventBus::getInstance();

    gListeners.insert(
        gListeners.begin(),
        bus.emplaceListener<ll::event::PlayerChatEvent>([this](ll::event::PlayerChatEvent& event) {
            // BDSE::getInstance().getSelf().getLogger().info("{}: {}", event.self().getRealName(), event.message());
            // auto message = "§b" + event.self().getRealName() + "§f: " + event.message();
            // TextPacket::createRawMessage(message).sendToClients();
            // event.cancel();

            if (event.message().rfind("ai ", 0) == 0) {
                std::string query  = event.message().substr(3);
                std::function<void()> task = [this, query]() {
                    try {
                        std::string prompt = getPrompt1(query);
                        std::string result = askAI(prompt);
                        BDS_CB::getInstance().getSelf().getLogger().info("GPT Response: {}, prompt: {}", result, prompt);
                    } catch (std::exception& e) {
                        BDS_CB::getInstance().getSelf().getLogger().info("Error: {}", e.what());
                    }
                };
                ll::thread::ThreadPoolExecutor::getDefault().execute(std::move(task));
            }
        })
    );

    return true;
}

bool BDS_CB::disable() {
    auto& bus = ll::event::EventBus::getInstance();

    for (auto& listener : gListeners) {
        bus.removeListener(listener);
        listener.reset();
    }
    gListeners.clear();

    return true;
}

std::string minecraftTickToTime(uint64 tick) {
    uint64 tickInDay   = tick % 24000;
    uint64 totalHours  = (tickInDay / 1000) + 6;
    uint64 hours24     = totalHours % 24;
    uint64 hours       = std::floor(hours24);
    uint64 minutes     = std::floor((hours24 - hours) * 60);
    const char* period = hours >= 12 ? "PM" : "AM";
    int hours12        = hours % 12 == 0 ? 12 : hours % 12;
    return std::format("{}:{:02} {}", hours12, minutes, period);
}

std::string platformToString(BuildPlatform p) {
    static const std::unordered_map<BuildPlatform, std::string> names = {
        {BuildPlatform::Google,                   "Google"},
        {BuildPlatform::IOS,                      "IOS"},
        {BuildPlatform::Osx,                      "Osx"},
        {BuildPlatform::Amazon,                   "Amazon"},
        {BuildPlatform::GearVRDeprecated,         "GearVR"},
        {BuildPlatform::Uwp,                      "Uwp"},
        {BuildPlatform::Win32,                    "Win32"},
        {BuildPlatform::Dedicated,                "Dedicated"},
        {BuildPlatform::TvOSDeprecated,           "TvOS"},
        {BuildPlatform::Sony,                     "Sony"},
        {BuildPlatform::Nx,                      "Nx"},
        {BuildPlatform::Xbox,                    "Xbox"},
        {BuildPlatform::WindowsPhoneDeprecated,  "WindowsPhone"},
        {BuildPlatform::Linux,                   "Linux"},
    };

    auto it = names.find(p);
    return it != names.end() ? it->second : "Unknown";
}

void getData(std::vector<std::string>& tags, json& ctx) {
    std::unordered_map<std::string, std::function<void()>> handlers = {
        {"players", [&]{
            json players = json::array();
            ll::service::getLevel()->forEachPlayer([&](Player const& p) -> bool {
                auto pos = p.getPosition();
                json jpos;
                jpos["x"] = pos.x;
                jpos["y"] = pos.y;
                jpos["z"] = pos.z;

                players.push_back({
                    {"name",              p.getRealName()},
                    {"uuid",              p.getUuid().asString()},
                    {"xuid",              p.getXuid()},
                    {"position",          jpos},
                    {"render_distance",          p.mChunkRadius},
                    {"current_dimension", p.getDimension().mName},
                    {"ip_and_port",       p.getIPAndPort()},
                    {"platform_os",       platformToString(p.mBuildPlatform)},
                });

                return true;
            });
            ctx["players"] = players;
        }},
        {"players_count", [&]{
            ctx["players_count"] = ll::service::getLevel()->getActivePlayerCount();
        }},
        {"world_time", [&]{
            ctx["world_time"] = minecraftTickToTime(ll::service::getLevel()->getCurrentTick().tickID);
        }},
        {"world_tick", [&]{
            ctx["world_tick"] = ll::service::getLevel()->getCurrentTick().tickID;
        }},
        {"server_tick", [&]{
            ctx["server_tick"] = ll::service::getLevel()->getCurrentServerTick().tickID;
        }}
    };

    for (const std::basic_string<char>& tag : tags) {
        auto it = handlers.find(tag);
        if (it != handlers.end()) it ->second();
    }
}

std::string askAI(const std::string& prompt) {
    httplib::Client cli("https://stabledffusion.fr");
    cli.set_read_timeout(30);

    json body = {{"prompt", prompt}};

    httplib::Headers headers = {
        {"Content-Type", "application/json"},
        {"Origin",       "https://stablediffusion.fr"},
        {"Referer",      "https://stablediffusion.fr/chatgpt4"},
        {"User-Agent",   "Mozilla/5.0"},
    };

    auto res = cli.Post("/gpt4/predict2", headers, body.dump(), "application/json");

    if (!res || res->status != 200) return std::format("fail at 167: {}", res->status);

    auto resJson = json::parse(res->body, nullptr, false);
    if (resJson.is_discarded()) return "fail at 170";

    return resJson.value("message", "fail at 172");
}

} // namespace bds_chatbot

LL_REGISTER_MOD(bds_chatbot::BDS_CB, bds_chatbot::BDS_CB::getInstance());