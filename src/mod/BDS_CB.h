#pragma once

#include "ll/api/mod/NativeMod.h"

namespace bds_chatbot {

class BDS_CB {

public:
    static BDS_CB& getInstance();

    BDS_CB() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    std::string getPrompt1(std::string question) {
        return std::format(R"(--------------- THE BEGINNING OF TEMPLATE ---------------
You are data requirement analyzer for a Minecraft Bedrock server assistant.

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
"how many players online?" -> ["player_count"]
"hello!" -> []
"what is 2+2?" -> []

If no server data is needed (greetings, general, non-server related questions, etc.), respond with: []

Rules:
- Respond with JSON array only, no explanation, no markdown, no emojis.
- Only use tags from the available list above.
- When question involves any player position or identity, always respond with "players"

--------------- THE END OF TEMPLATE ---------------

Player's question: {})", question);
    }

    
    std::string getPrompt2(std::string datas, std::string sender, std::string question) {
        return std::format(R"(--------------- THE BEGINNING OF TEMPLATE ---------------
You are a friendly assistant for a Minecraft Bedrock server named "AnomalySurvival"
With server IP Address of "play.skyes.me" and Port of "25600"

Server data (may be empty if not relevant):
{}

Rules:
- Be concise, no long text.
- No emojis.
- NO "based on the data" or similar phrases.
- If asked about coordinates, answer with x, y, z format.
- Answer in thye same language of the player used.
- If no server data provided, just responsd naturally, like a normal conversation.

--------------- THE END OF TEMPLATE ---------------

You're talking with: {}
Player's question: {})", datas, sender, question);
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
};

} // namespace bds_chatbot