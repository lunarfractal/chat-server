#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <unordered_map>
#include <string>

namespace utils {
uint16_t getUniqueId() {
    static uint16_t current = 1;
    return current++;
}

std::unordered_map<std::string, std::string> parse_query(const std::string& query_string) {
    std::unordered_map<std::string, std::string> query_map;

    if (query_string.empty()) {
        return query_map;
    }

    size_t start = 0;
    while (start < query_string.length()) {
        size_t end = query_string.find('&', start);
        if (end == std::string::npos) {
            end = query_string.length();
        }

        std::string pair = query_string.substr(start, end - start);

        size_t equal_pos = pair.find('=');
        if (equal_pos == std::string::npos) {
            if (!pair.empty()) {
                query_map[pair] = "";
            }
        } else {
            std::string key = pair.substr(0, equal_pos);
            std::string value = pair.substr(equal_pos + 1);
            query_map[key] = value;
        }

        start = end + 1;
    }

    return query_map;
}

}

#endif
