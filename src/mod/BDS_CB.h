#pragma once

#include "Config.h"
#include <ll/api/mod/NativeMod.h>

#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <deque>
#include <unordered_map>

namespace bds_chatbot {

using json = nlohmann::json;

struct ChatMessage {
    std::string role;
    std::string content;
};

class BDS_CB {

public:
    static BDS_CB& getInstance();

    BDS_CB() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    std::string getTemplate1() {
        return template1;
    }

    std::string getTemplate2(const std::string& datas) {
        return std::vformat(template2, std::make_format_args(
            mConfig.server_name,
            mConfig.server_ip,
            mConfig.server_port,
            datas
        ));
    }

    json getHistory() {
        json j = json::array();
        for (auto& msg : sharedHistory) {
            j.push_back({ {"role", msg.role}, {"content", msg.content} });
        }
        return j;
    }

    void addToHistory(const std::string& playerName, const std::string& role, const std::string& content) {
        std::string formatted = content;

        if (role == "user") {
            formatted = "(" + playerName + "): " + content;
        }

        sharedHistory.push_back({ role, formatted });

        while (sharedHistory.size() > 50) {
            sharedHistory.pop_front();
        }
    }

    void saveHistory() {
        auto path = getSelf().getConfigDir() / "chat_history.json";

        json j = json::array();
        for (auto& msg : sharedHistory) {
            j.push_back({ {"role", msg.role}, {"content", msg.content} });
        }

        std::ofstream f(path);
        f << j.dump(2);
    }

    void loadHistory() {
        auto path = getSelf().getConfigDir() / "chat_history.json";
        if (!std::filesystem::exists(path)) return;

        std::ifstream f(path);
        json j = json::parse(f, nullptr, false);
        if (j.is_discarded() || !j.is_array()) return;

        sharedHistory.clear();
        for (auto& item : j) {
            sharedHistory.push_back({
                item.value("role", ""),
                item.value("content", "")
            });
        }

        while (sharedHistory.size() > 50) {
            sharedHistory.pop_front();
        }
    }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    // TODO: Implement this method if you need to unload the mod.
    // /// @return True if the mod is unloaded successfully.
    // bool unload();

private:
    ll::mod::NativeMod& mSelf;
    Config mConfig;
    std::deque<ChatMessage> sharedHistory;

    std::string template1 = R"(You are data requirement analyzer for a Minecraft Bedrock server assistant.

Available data flags:
"players": player list which contains name, uuid, xuid, platform_os, coordinate, current_dimension, ip_and_port and render_distance.
"players_count": total connected player in server.
"world_time": world time, like 4:36 PM.
"world_tick": current world tick.
"server_tick": current server tick.

Given the player's question, response ONLY with a JSON array of type tags needed to answer it.

Examples:
"where is Foo?" -> ["players"]
"where am I?" -> ["players"]
"what's my location?" -> ["players"]
"what time it is?" -> ["world_time"]
"what's current server tick?" -> ["server_tick"]
"how many players online?" -> ["players_count"]
"hello!" -> []
"what is 2+2?" -> []

If no server data is needed (greetings, general, non-server related questions, etc.), respond with: []

Rules:
- Respond with JSON array only, no explanation, no markdown, no emojis.
- Only use tags from the available list above.
- When question involves any player position or identity, always respond with "players")";

    std::string template2 = R"(You are a friendly assistant for a Minecraft Bedrock server named "{}"
With server IP Address of "{}" and Port of "{}"

Server data (may be empty if not relevant):
{}

Rules:
- Be concise, no long text.
- No emojis.
- NO "based on the data" or similar phrases.
- If asked about coordinates, answer with x, y, z format.
- Answer in the same language of the player used.
- If no server data provided, just respond naturally, like a normal conversation.)";

};

} // namespace bds_chatbot