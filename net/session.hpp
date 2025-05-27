#ifndef SESSION_HPP
#define SESSION_HPP

#include <websocketpp/common/connection_hdl.hpp>
#include "../game/player.hpp"
#include "../game/world.hpp"

typedef websocketpp::connection_hdl connection_hdl;

namespace game { struct Player; }

namespace net {

struct session {
    connection_hdl hdl;

    bool sent_ping = false;
    bool sent_hello = false;
    uint16_t sent_nick_count = 0;

    bool needsInitPackets = true;
    uint16_t screen_width = 0, screen_height = 0;

    std::shared_ptr<game::Player> player;

    bool did_enter_game() const {
        return player != nullptr;
    }
};

}

#endif
