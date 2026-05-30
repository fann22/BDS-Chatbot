#include "mod/BDS_CB.h"

#include <ll/api/service/Bedrock.h>

#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerChatEvent.h>

namespace bds_chatbot {

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
        bus.emplaceListener<ll::event::PlayerChatEvent>([](ll::event::PlayerChatEvent& event) {
            // BDSE::getInstance().getSelf().getLogger().info("{}: {}", event.self().getRealName(), event.message());
            // auto message = "§b" + event.self().getRealName() + "§f: " + event.message();
            // TextPacket::createRawMessage(message).sendToClients();
            // event.cancel();
            if (event.message().rfind("ai ", 0) == 0) {
                std::string query = event.message().substr(3);
                BDS_CB::getInstance().getSelf().getLogger().info("Prompt from {}: {}", event.self().getRealName(), query);
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

} // namespace bds_chatbot

LL_REGISTER_MOD(bds_chatbot::BDS_CB, bds_chatbot::BDS_CB::getInstance());