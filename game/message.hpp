#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <unordered_map>

namespace game {

struct Message {
    std::u16string content;
    std::u16string owner_nick;
    uint16_t owner_hue;
    uint16_t owner_id;
    double timestamp;
};

}

#endif
