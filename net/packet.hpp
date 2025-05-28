#ifndef PACKET_HPP
#define PACKET_HPP

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

namespace net {
class packetw {
public:
    packetw(int size);

    packetw& i8(int8_t v);
    packetw& i16(int16_t v);
    packetw& i32(int32_t v);
    packetw& i64(int64_t v);
    packetw& u8(uint8_t v);
    packetw& u16(uint16_t v);
    packetw& u32(uint32_t v);
    packetw& u64(uint64_t v);
    packetw& f32(float v);
    packetw& f64(double v);
    packetw& string(const std::string &v);
    packetw& u16string(const std::u16string &v);

    uint8_t* data();
private:
    int offset;
    std::vector<uint8_t> buffer;
};

class packetr {
public:
    packetr(std::string &s);

    int8_t i8();
    int16_t i16();
    int32_t i32();
    int64_t i64();
    uint8_t u8();
    uint16_t u16();
    uint32_t u32();
    uint64_t u64();
    float f32();
    double f64();
    std::string string();
    std::u16string u16string();
private:
    int offset;
    std::string buffer;
};

packetw::packetw(int size) : offset(0), buffer(size) {}

packetw& packetw::i8(int8_t v) {
    std::memcpy(&buffer[offset], &v, sizeof(int8_t));
    offset += sizeof(int8_t);
    return *this;
}

packetw& packetw::i16(int16_t v) {
    std::memcpy(&buffer[offset], &v, sizeof(int16_t));
    offset += sizeof(int16_t);
    return *this;
}

packetw& packetw::i32(int32_t v) {
    std::memcpy(&buffer[offset], &v, sizeof(int32_t));
    offset += sizeof(int32_t);
    return *this;
}

packetw& packetw::i64(int64_t v) {
    std::memcpy(&buffer[offset], &v, sizeof(int64_t));
    offset += sizeof(int64_t);
    return *this;
}

packetw& packetw::u8(uint8_t v) {
    std::memcpy(&buffer[offset], &v, sizeof(uint8_t));
    offset += sizeof(uint8_t);
    return *this;
}

packetw& packetw::u16(uint16_t v) {
    std::memcpy(&buffer[offset], &v, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    return *this;
}

packetw& packetw::u32(uint32_t v) {
    std::memcpy(&buffer[offset], &v, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return *this;
}

packetw& packetw::u64(uint64_t v) {
    std::memcpy(&buffer[offset], &v, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    return *this;
}

packetw& packetw::f32(float v) {
    std::memcpy(&buffer[offset], &v, sizeof(float));
    offset += sizeof(float);
    return *this;
}

packetw& packetw::f64(double v) {
    std::memcpy(&buffer[offset], &v, sizeof(double));
    offset += sizeof(double);
    return *this;
}

packetw& packetw::string(const std::string &v) {
    std::memcpy(&buffer[offset], v.data(), v.length());
    offset += v.length();
    buffer[offset++] = 0x00;
    return *this;
}

packetw& packetw::u16string(const std::u16string &v) {
    std::memcpy(&buffer[offset], v.data(), 2 * v.length());
    offset += 2 * v.length();
    buffer[offset++] = 0x00;
    buffer[offset++] = 0x00;
    return *this;
}

uint8_t* packetw::data() {
    return buffer.data();
}

packetr::packetr(std::string &s) : buffer(s), offset(0) {}

int8_t packetr::i8() {
    int8_t v;
    std::memcpy(&v, &buffer[offset], sizeof(int8_t));
    offset += sizeof(int8_t);
    return v;
}

int16_t packetr::i16() {
    int16_t v;
    std::memcpy(&v, &buffer[offset], sizeof(int16_t));
    offset += sizeof(int16_t);
    return v;
}

int32_t packetr::i32() {
    int32_t v;
    std::memcpy(&v, &buffer[offset], sizeof(int32_t));
    offset += sizeof(int32_t);
    return v;
}

int64_t packetr::i64() {
    int64_t v;
    std::memcpy(&v, &buffer[offset], sizeof(int64_t));
    offset += sizeof(int64_t);
    return v;
}

uint8_t packetr::u8() {
    uint8_t v;
    std::memcpy(&v, &buffer[offset], sizeof(uint8_t));
    offset += sizeof(uint8_t);
    return v;
}

uint16_t packetr::u16() {
    uint16_t v;
    std::memcpy(&v, &buffer[offset], sizeof(uint16_t));
    offset += sizeof(uint16_t);
    return v;
}

uint32_t packetr::u32() {
    uint32_t v;
    std::memcpy(&v, &buffer[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return v;
}

uint64_t packetr::u64() {
    uint64_t v;
    std::memcpy(&v, &buffer[offset], sizeof(uint64_t));
    offset += sizeof(uint64_t);
    return v;
}

float packetr::f32() {
    float v;
    std::memcpy(&v, &buffer[offset], 4);
    offset += 4;
    return v;
}

double packetr::f64() {
    double v;
    std::memcpy(&v, &buffer[offset], 8);
    offset += 8;
    return v;
}

std::string packetr::string() {
    std::string dest;
    const size_t len = buffer.size();

    while (offset < len) {
        char ch = buffer[offset++];
        if (ch == '\0') break;
        dest.push_back(ch);
    }

    return dest;
}

std::u16string packetr::u16string() {
    std::u16string dest;
    const size_t len = buffer.size();

    while (offset + 1 < len) {
        char16_t ch;
        std::memcpy(&ch, &buffer[offset], sizeof(char16_t));
        offset += 2;

        if (ch == u'\0') break;
        dest.push_back(ch);
    }

    return dest;
}
}

#endif
