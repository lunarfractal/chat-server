#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstring>

#include "game/world.hpp"
#include "net/session.hpp"
#include "net/opcodes.hpp"
#include "utils/utils.hpp"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

typedef websocketpp::server<websocketpp::config::asio> server;
typedef websocketpp::connection_hdl connection_hdl;
typedef server::message_ptr message_ptr;


class WebSocketServer {
    public:
        WebSocketServer() {
            m_server.init_asio();

            m_server.set_open_handler(bind(&WebSocketServer::on_open,this,::_1));
            m_server.set_close_handler(bind(&WebSocketServer::on_close,this,::_1));
            m_server.set_message_handler(bind(&WebSocketServer::on_message,this,::_1,::_2));

            m_server.clear_access_channels(websocketpp::log::alevel::all);
        }


        void processMessage(std::string &buffer, connection_hdl hdl) {
            auto it = m_sessions.find(hdl);
            if(it == m_sessions.end()) return;
            auto s = it->second;

            uint8_t op = buffer[0];

            switch(op) {
                case net::opcode_ping:
                {
                    pong(hdl);
                    if(!s->sent_ping) s->sent_ping = true;
                    break;
                }

                case net::opcode_hi:
                {
                    if(buffer.length() >= 5) {
                        std::memcpy(&s->screen_width, &buffer[1], 2);
                        std::memcpy(&s->screen_height, &buffer[3], 2);
                    }
                    if(!(s->sent_hello)) s->sent_hello = true;

                    break;
                }

                case net::opcode_hi_bot:
                break;

                case net::opcode_enter_game:
                {
                    if(s->did_enter_game() || !(s->sent_ping) || !(s->sent_hello)) return;

                    auto player = std::make_shared<game::Player>();
                    player->session = s;

                    int offset = 1;
                    player->nick = utils::getU16String(buffer, offset);

                    player->id = game_world.add_player(player);

                    std::string room_id = s->orig_room_id;

                    if(game_world.rooms.find(room_id) == game_world.rooms.end()) {
                        game_world.rooms.insert(room_id);
                    }
                    
                    player->room_id = room_id;

                    sendId(hdl, player->id);

                    s->player = player;

                    s->sent_nick_count++;
/*
                    std::cout << "Entered game: " << (int)s->player->id << std::endl
                        << "Size of map: " << gameWorld.activePlayers.size() << std::endl; */
                    break;
                }

                case net::opcode_leave_game:
                {
                    if(!(s->did_enter_game())) return;

                  /*  std::cout << "received opcode leave game" << std::endl;*/

                    s->player->deletion_reason = 0x03;
                    game_world.mark_for_deletion(s->player->id);
                    break;
                }

                case net::opcode_resize:
                {
                    if(buffer.length() >= 5) {
                        std::memcpy(&s->screen_width, &buffer[1], 2);
                        std::memcpy(&s->screen_height, &buffer[3], 2);
                    }

                    break;
                }

                case net::opcode_cursor:
                {
                    if(!s->did_enter_game()) return;

                    uint16_t x, y;
                    std::memcpy(x, &buffer[1], 2);
                    std::memcpy(&y, &buffer[3], 2);
 /*                   std::cout << "update cursor: " << (int)x << " " << (int)y << std::endl;*/
                    s->player->updateCursor(x, y);
                    break;
                }

                case net::opcode_cd:
                {
                    if(!s->did_enter_game()) return;
                    int offset = 1;
                    std::string room_id = utils::getString(buffer, offset);

                    if(game_world.rooms.find(room_id) == game_world.rooms.end()) {
                        game_world.rooms.insert(room_id);
                    }

                    s->player->room_id = room_id;

                    break;
                }

                case net::opcode_ls:
                break;

                case net::opcode_chat:
                {
                    int offset = 1;
                    std::u16string chat_message = utils::getU16String(buffer, offset);
                    dispatch_event(chat_message, s->player->id, s->player->roon_id);
                    break;
                }

                default:
                break;
            }
        }

        void cycle_s() {
            std::thread([this]() {
            while(true) {
                auto then = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000 / 30);

                if(game_world.active_players.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                   /* std::cout << "Sleeping... " << gameWorld.activePlayers.size()
                        << std::endl; */
                    continue;
                }

                // update to every player
                for(auto &[id, player]: game_world.active_players) {
                    // so, is this specific player entering the game for the first time?
                    if(player->session->sent_nick_count == 1 && player->session->needsInitPackets && player->deletion_reason == 0) {
                      /*  std::cout << "first time entering game" << std::endl;*/
                        int bufferSize = 1;

                        for(auto &pair: game_world.active_players) {
                            if(pair.second->id == id) {
                              /*  std::cout << "id = my id" << std::endl;*/
                                continue;
                            }
                            bufferSize += 2; // id
                            bufferSize += 1; // flag
                            bufferSize += 4; // 2+2 xy
                            bufferSize += 2 * pair.second->nick.length() + 2;
                            bufferSize += 1; // reason
                        }

                        bufferSize += 2;

                        std::vector<uint8_t> buffer(bufferSize);
                        buffer[0] = net::opcode_cycle_s;

                        int offset = 1;

                        for(auto &pair: game_world.active_players) {
                            if(pair.second->id == id) {
                              /*  std::cout << "it's my id" << std::endl;*/
                                continue;
                            }
                            std::memcpy(&buffer[offset], &pair.first, 2);
                            offset += 2;
                            buffer[offset++] = 0x0;
                            std::memcpy(&buffer[offset], &pair.second->x, 2);
                            offset += 2;
                            std::memcpy(&buffer[offset], &pair.second->y, 2);
                            offset += 2;
                            std::memcpy(&buffer[offset], pair.second->nick.data(), 2 * pair.second->nick.length());
                            offset += 2 * pair.second->nick.length();
                            buffer[offset++] = 0x00;
                            buffer[offset++] = 0x00;
                            buffer[offset++] = 0x02;
                            player->view.insert(pair.first);
                        }

                        buffer[offset++] = 0x00;
                        buffer[offset++] = 0x00;

                        sendBuffer(player->session->hdl, buffer.data(), buffer.size());

                        player->session->needsInitPackets = false;
                      /*  std::cout << "Sent init packets" << std::endl;*/
                        continue;
                    }

                    int bufferSize = 1; // opcode

                    for(auto &pair: game_world.active_players) {
                      /*  std::cout << "trying to size normal packet" << std::endl;*/
                        if(pair.second->id == id) {
                          /*  std::cout << "its my id" << std::endl;*/
                            continue;
                        }
                        if(!player->hasInView(pair.second)) continue;
                        if(pair.second->deletion_reason > 0) {
                            bufferSize += 2; // id
                            bufferSize += 1; // flag
                            bufferSize += 1; // killReason
                            continue;
                        }
                        uint8_t creation = player->view.find(pair.second->id) == player->view.end() ? 0x0 : 0x1;

                        bufferSize += 2; // id
                        bufferSize += 1; // flag
                        bufferSize += 4; // 2+2 uint16's
                        if(creation == 0x0) bufferSize += 2 * pair.second->nick.length() + 2 + 1;
                    }

                    bufferSize += 2;

                    std::vector<uint8_t> buffer(bufferSize);

                    buffer[0] = net::opcode_cycle_s);

                    int offset = 1;
                    
                    for(auto &pair: game_world.active_players) {
                      /*  std::cout << "trying to encode normal packet" << std::endl;*/
                        if(pair.first == id) {
                            continue;
                        }
                        if(pair.second->deletion_reason == 0x02) {
                            std::memcpy(&buffer[offset], &pair.first, 2);
                            offset += 2;
                            buffer[offset++] = 0x2;
                            buffer[offset++] = 0x02;
                            continue;
                        } else if(pair.second->deletion_reason == 0x03) {
                            std::memcpy(&buffer[offset], &pair.first, 2);
                            offset += 2;
                            buffer[offset++] = 0x2;
                            buffer[offset++] = 0x03;
                            continue;
                        }

                        if(player->hasInView(pair.second)) {
                            uint8_t reason;
                            uint8_t creation = player->view.find(pair.second->id) == player->view.end() ? 0x0 : 0x1;
                            std::memcpy(&buffer[offset], &pair.first, 2);
                            offset += 2;
                            buffer[offset++] = creation;
                            std::memcpy(&buffer[offset], &pair.second->x, 2);
                            offset += 2;
                            std::memcpy(&buffer[offset], &pair.second->y, 2);
                            offset += 2;
                            if(creation == 0x0) {
                                if(pair.second->session->sent_nick_count == 1) reason = 0x00;
                                else if(pair.second->session->sent_nick_count > 1) reason = 0x01;
                                player->view.insert(pair.second->id);
                                std::memcpy(&buffer[offset], pair.second->nick.data(), 2 * pair.second->nick.length());
                                offset += 2 * pair.second->nick.length();
                                buffer[offset++] = 0x00;
                                buffer[offset++] = 0x00;
                                buffer[offset++] = reason;
                            }
                        } else {
                            if(player->view.find(pair.second->id) != player->view.end()) {
                                player->view.erase(pair.second->id);
                                std::memcpy(&buffer[offset], &pair.first, 2);
                                offset += 2;
                                buffer[offset++] = 0x2;
                                buffer[offset++] = 0x1;
                            }
                        }
                   }

                   buffer[offset++] = 0x00;
                   buffer[offset++] = 0x00;

                   sendBuffer(player->session->hdl, buffer.data(), buffer.size());
                }

                for(uint16_t id: game_world.pending_deletions) {
                    game_world.delete_player(id);
                    game_world.pending_deletions.erase(id);
                }
                std::this_thread::sleep_until(then);
            }
            }).detach();
        }

        void on_open(connection_hdl hdl) {
            server::connection_ptr con = m_server.get_con_from_hdl(hdl);
            std::string path = con->get_resource();

            std::string room_id = "lobby";

            std::unordered_map<std::string, std::string> query = utils::parse_query(path);

            auto it = query.find("id");
            if(it != query.end()) {
                room_id = it->second;
            }
            
            auto s = std::make_shared<net::session>();
            s->hdl = hdl;
            s->orig_room_id = room_id;
            m_sessions[hdl] = s;
        }

        void on_close(connection_hdl hdl) {
            auto it = m_sessions.find(hdl);
            if(it == m_sessions.end()) return;

            if(it->second->did_enter_game()) {
                it->second->player->deletion_reason = 0x02;
                game_world.mark_for_deletion(it->second->player->id);
            }
            m_sessions.erase(hdl);
        }

        void on_message(connection_hdl hdl, message_ptr msg) {
            if(msg->get_opcode() == websocketpp::frame::opcode::binary) {
                std::string payload = msg->get_payload();
                processMessage(payload, hdl);
            }
        }

        void run(uint16_t port) {
            m_server.listen(port);
            m_server.start_accept();
            m_server.run();
        }
    private:
        game::GameWorld game_world;

        server m_server;

        typedef struct {
            std::size_t operator()(const websocketpp::connection_hdl& hdl) const {
                return std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(hdl.lock().get()));
            }
        } connection_hdl_hash;
        typedef struct {
            bool operator()(const websocketpp::connection_hdl& lhs, const websocketpp::connection_hdl& rhs) const {
                return !lhs.owner_before(rhs) && !rhs.owner_before(lhs);
            }
        } connection_hdl_equal;

        std::unordered_map<connection_hdl, std::shared_ptr<net::session>, connection_hdl_hash, connection_hdl_equal> m_sessions;


        void dispatch_message(const std::u16string &value, uint16_t id, const std::string &room_id) {
            const int size = 1 + 1 + 2 + 2 * value.length() + 2);
            std::vector<uint8_t> buffer(size);
            buffer[0] = net::opcode_events;
            int offset = 1;
            buffer[offset++] = 0x1;
            std::memcpy(&buffer[offset], &id, 2);
            offset += 2;
            std::memcpy(&buffer[offset], value.data(), 2 * value.length());
            offset += 2 * value.length();
            buffer[offset++] = 0x00;
            buffer[offset++] = 0x00;
                    
            send_dispatch(buffer.data(), buffer.size(), room_id);
        }

        void pong(connection_hdl hdl) {
            uint8_t buffer[] = {net::opcode_pong};
            try {
                m_server.send(hdl, buffer, sizeof(buffer), websocketpp::frame::opcode::binary);
            } catch (websocketpp::exception const & e) {
                std::cout << "Pong failed because: "
                    << "(" << e.what() << ")" << std::endl;
            }
        }

        void sendId(connection_hdl hdl, uint16_t id) {
            uint8_t buffer[3];
            buffer[0] = net::opcode_entered_game;
            std::memcpy(&buffer[1], &id, sizeof(uint16_t));
            try {
                m_server.send(hdl, buffer, sizeof(buffer), websocketpp::frame::opcode::binary);
            } catch (websocketpp::exception const & e) {
                std::cout << "Send failed because: "
                    << "(" << e.what() << ")" << std::endl;
            }
        }

        void send_dispatch(uint8_t* buffer, size_t size, std::string &room_id) {
            for (auto &pair: m_sessions) {
                try {
                    if (
                        m_server.get_con_from_hdl(pair.first)->get_state() == websocketpp::session::state::open
                        && pair.second->sent_ping
                        && pair.second->sent_hello
                        && pair.second->player->room_id == room_id
                    ) {
                        m_server.send(pair.first, buffer, size, websocketpp::frame::opcode::binary);
                    }
                } catch (websocketpp::exception const & e) {
                    std::cout << "Send failed because: "
                        << "(" << e.what() << ")" << std::endl;
                }
            }
        }

        void sendAll(uint8_t* buffer, size_t size) {
            for (auto &pair: m_sessions) {
                try {
                    if (
                        m_server.get_con_from_hdl(pair.first)->get_state() == websocketpp::session::state::open
                        && pair.second->sent_ping
                        && pair.second->sent_hello
                    ) {
                        m_server.send(pair.first, buffer, size, websocketpp::frame::opcode::binary);
                    }
                } catch (websocketpp::exception const & e) {
                    std::cout << "Send failed because: "
                        << "(" << e.what() << ")" << std::endl;
                }
            }
        }

        void sendBuffer(connection_hdl hdl, uint8_t *buffer, size_t size) {
            try {
                if (
                        m_server.get_con_from_hdl(hdl)->get_state() == websocketpp::session::state::open
                )
                    m_server.send(hdl, buffer, size, websocketpp::frame::opcode::binary);
            } catch (websocketpp::exception const & e) {
                std::cout << "Send failed because: "
                    << "(" << e.what() << ")" << std::endl;
            }
        }
};

int main() {
    WebSocketServer wsServer;
    wsServer.cycle_s();
    wsServer.run(8081);
}
