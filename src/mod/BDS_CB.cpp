#include "mod/BDS_CB.h"
#include "ll/api/Config.h"

#include <nlohmann/json.hpp>
#include <openssl/ssl.h>

#include <ll/api/Config.h>
#include <ll/api/service/Bedrock.h>
#include <ll/api/mod/RegisterHelper.h>

#include <ll/api/event/EventBus.h>
#include <ll/api/thread/ThreadPoolExecutor.h>
#include <ll/api/event/player/PlayerChatEvent.h>

#include <mc/deps/core/platform/BuildPlatform.h>
#include <mc/world/Level/level.h>
#include <mc/world/Level/dimension/Dimension.h>
#include <mc/world/actor/player/Player.h>

#include <mc/network/packet/TextPacket.h>

#include <unordered_map>

#include "mod/httplib.h"

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
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    const auto& configPath = getSelf().getConfigDir() / "config.json";
    if (!ll::config::loadConfig(BDS_CB::getInstance().mConfig, configPath)) {
        BDS_CB::getInstance().getSelf().getLogger().warn("Cannot load config, saving defaults.");
    }

    loadHistory();

    return true;
}

bool BDS_CB::enable() {
    auto& bus = ll::event::EventBus::getInstance();

    gListeners.insert(
        gListeners.begin(),
        bus.emplaceListener<ll::event::PlayerChatEvent>([this](ll::event::PlayerChatEvent& event) {
            if (event.message().rfind("ai ", 0) == 0) {
                std::string sender = event.self().getRealName() ;
                std::string query  = event.message().substr(3);
                std::function<void()> task = [this, sender, query]() {
                    try {
                        std::string response1 = askAI(getTemplate1(), sender, query, json::array());

                        auto tagsJson = json::parse(response1, nullptr, false);
                        if (tagsJson.is_discarded() || !tagsJson.is_array()) throw std::runtime_error("JSON parse failed.");

                        std::vector<std::string> tags = tagsJson.get<std::vector<std::string>>();

                        json ctx;
                        getData(tags, ctx);

                        json history = getHistory();
                        std::string response2 = askAI(getTemplate2(ctx.dump(1)), sender, query, history);
                        TextPacket::createRawMessage(std::format("[§l§dAI§r]: {}", response2)).sendToClients();

                        addToHistory(sender, "user", query);
                        addToHistory(sender, "assistant", response2);
                    } catch (std::exception& e) {
                        BDS_CB::getInstance().getSelf().getLogger().info("Error: {}", e.what());
                        TextPacket::createRawMessage(std::format("[§l§dAI§r]: §4Error: {}", e.what())).sendToClients();
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

    saveHistory();
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
                    {"render_distance",   p.mChunkRadius},
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

std::string askAI(const std::string& _template, const std::string& sender, const std::string& prompt, const json& history) {
    httplib::SSLClient cli("g4f.space");
    cli.set_read_timeout(30);

    json messages = json::array();

    messages.push_back({
        {"role", "system"},
        {"content", _template},
    });

    for (const auto& msg : history) {
        messages.push_back(msg);
    }

    messages.push_back({
        {"role", "user"},
        {"content", "(" + sender + "): " + prompt},
    });

    json body = {
        {"model", "openai/gpt-oss-120b"},
        {"temperature", 0.6},
        {"messages", messages},
    };

    auto res = cli.Post(
        "/api/groq/chat/completions",
        body.dump(),
        "application/json"
    );

    if (!res) throw std::runtime_error(std::format("HTTP error: {}.", httplib::to_string(res.error())));
    if (res->status != 200) throw std::runtime_error(std::format("HTTP status: {}.", res->status));

    auto resJson = json::parse(res->body, nullptr, false);
    if (resJson.is_discarded()) throw std::runtime_error("JSON parse failed.");
    if (!resJson.contains("choices") || resJson["choices"].empty()) throw std::runtime_error("Empty JSON result.");

    auto reply = resJson["choices"][0]["message"].value("content", "");
    if (reply.isEmpty()) throw std::runtime_error("AI responded with empty message.");
    return reply;
}

} // namespace bds_chatbot

LL_REGISTER_MOD(bds_chatbot::BDS_CB, bds_chatbot::BDS_CB::getInstance());