#ifndef OPCODES_HPP
#define OPCODES_HPP

#include <cstdint>

namespace net {

constexpr uint8_t opcode_ping = 0x00;
constexpr uint8_t opcode_hi = 0x01;
constexpr uint8_t opcode_hi_bot = 0x02;
constexpr uint8_t opcode_enter_game = 0x03;
constexpr uint8_t opcode_leave_game = 0x04;
constexpr uint8_t opcode_resize = 0x05;
constexpr uint8_t opcode_cursor = 0x06;
constexpr uint8_t opcode_cd = 0x07;
constexpr uint8_t opcode_ls = 0x08;
constexpr uint8_t opcode_chat = 0x09;
constexpr uint8_t opcode_ls_messages = 0x10;

constexpr uint8_t opcode_pong = 0x00;
constexpr uint8_t opcode_entered_game = 0xA0;
constexpr uint8_t opcode_cycle_s = 0xA4;
constexpr uint8_t opcode_events = 0xA1;

}

#endif
