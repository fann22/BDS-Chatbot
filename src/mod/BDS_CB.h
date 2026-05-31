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

Available data flags: "players", "players_count", "world_time", "world_tick", "server_tick"

Given the player's question, response ONLY with a JSON array of type tags needed to answer it.
Example: ["players", "world_time"]

If no server data is needed (greetings, general, non-server related questions, etc.), respond with: []

Rules:
- Respond with JSON array only, no explanation, no markdown, no emojis.
- Only use tags from the available list above.

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