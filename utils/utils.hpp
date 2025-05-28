#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>
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

std::u16string getU16String(const std::string& data, int &offset) {
    std::u16string result;

    if (offset >= data.size()) {
        throw std::out_of_range("offset is out of bounds");
    }

    for (; offset < data.size(); offset += 2) {
        char16_t ch = static_cast<char16_t>(data[offset] | (data[offset + 1] << 8));
        if (ch == u'\0') {
            break;
        }
        result.push_back(ch);
    }

    return result;
}

std::string getString(const std::string& data, int &offset) {
    std::string result;

    if (offset >= data.size()) {
        throw std::out_of_range("offset is out of bounds");
    }

    while (offset < data.size()) {
        char ch = data[offset++];
        if (ch == '\0') {
            return result;
        }
        result.push_back(ch);
    }

    throw std::runtime_error("no null terminator");
}

}

#endif
