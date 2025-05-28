#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string>
#include <unordered_set>
#include "../net/session.hpp"

namespace net { struct session; }

namespace game {

struct Player {
    uint16_t x = 0, y = 0;
    uint16_t id;
    std::string room_id;
    std::u16string nick;
    std::shared_ptr<net::session> session;

    uint8_t deletion_reason = 0;

    std::unordered_set<uint16_t> view;

    void updateCursor(uint16_t _x, uint16_t _y) {
        x = (_x * 65535) / session->screen_width;
        y = (_y * 65535) / session->screen_height;
    }

    bool hasInView(std::shared_ptr<Player> player0) {
        return player0->room_id == room_id;
    }
};

}

#endif
