#ifndef WORLD_HPP
#define WORLD_HPP

#include <deque>
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
    std::unordered_map<std::string, std::deque<Message>> id2messages;

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
        if (it == active_players.end()) return;

        auto playerPtr = it->second;

        if (playerPtr->session) {
            if (playerPtr->session->player) {
                playerPtr->session->player.reset();
            }
            playerPtr->session.reset();
         }

        active_players.erase(it);
    }

    void add_message(std::string& room_id, Message newMsg) {
        std::deque<Message> &messages = id2messages[room_id];

        if (messages.size() >= 100) {
            messages.pop_front();
        }
        messages.push_back(newMsg);
    }
};

}

#endif
