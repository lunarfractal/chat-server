#ifndef WORLD_HPP
#define WORLD_HPP

#include <unordered_map>
#include <unordered_set>
#include "../utils/utils.hpp"
#include "player.hpp"
#include "message.hpp"


namespace game {

class GameWorld {
public:
    std::unordered_map<uint16_t, std::shared_ptr<Player>> active_players;
    std::unordered_set<uint16_t> pending_deletions;
    std::unordered_set<std::string> rooms;
    std::unordered_map<std::string, std::unordered_set<Message>> id2messages;

    void mark_for_deletion(uint16_t id) {
        pending_deletions.insert(id);
    }

    uint16_t add_player(std::shared_ptr<game::Player> player) {
        uint16_t id = utils::getUniqueId();
        active_players[id] = player;
        return id;
    }

    void delete_player(uint16_t id) {
        auto it = active_players.find(id);
        if(it == active_players.end()) return;

        active_players.erase(id);
    }
};

}

#endif
