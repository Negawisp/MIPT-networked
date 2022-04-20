#include <enet/enet.h>
#include <cstring>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "common/constants.h"
#include "common/enet_tools.h"

const char* START_MESSAGE = "Start";


void send_start_packet(ENetPeer *peer)
{
  std::string message;
  message.push_back(FLAG_IS_START);
  message.append(START_MESSAGE);

  ENetPacket *packet = enet_packet_create(message.c_str(), message.length() + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 1, packet);
}


int main(int argc, const char **argv)
{
  bool is_connected = false;
  bool is_started = false;
  bool is_quit = false;

  uint32_t time_start = enet_time_get();
  uint32_t systime_send_timestamp = time_start;

  ENetAddress lobby_addr;
  ENetAddress server_addr;

  ENetPeer* lobby_peer;
  ENetPeer* server_peer;

  // Processing console args
  switch (argc) {
  case 1:
    enet_address_set_host(&lobby_addr, "localhost");
    lobby_addr.port = DEFAULT_LOBBY_PORT;
    break;
  case 2:
    enet_address_set_host(&lobby_addr, "localhost");
    lobby_addr.port = static_cast<enet_uint16>(std::atoi(argv[2]));
    break;
  case 3:
    enet_address_set_host(&lobby_addr, argv[1]);
    lobby_addr.port = static_cast<enet_uint16>(std::atoi(argv[2]));
    break;
  default:
    std::cerr << "Usage:\n" <<
                 " 1) " << argv[0] << " to connect to lobby at localhost:" << DEFAULT_LOBBY_PORT << "\n" <<
                 " 2) " << argv[0] << " <lobby port> to connect to lobby at localhost:<lobby port>" << "\n" <<
                 " 3) " << argv[0] << " <lobby addr> <lobby port> to connect to lobby at <lobby addr>:<lobby port>" << std::endl;
    return -1; 
  }

  // Initializing enet
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  atexit(enet_deinitialize);

  ENetHost* client_host = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client_host)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  // Connecting to lobby
  lobby_peer = enet_tool__connect(lobby_addr, client_host);

  // Making console input non-blocking
  std::mutex m;
  std::condition_variable cv_input_obtained;
  std::vector<std::string> console_input_queue;

  auto input_reading_and_sending = std::thread([&]{
      while(!is_quit) {
        // reading input
        std::string s;
        std::getline(std::cin, s, '\n');

        if (s == "quit") {
            is_quit = true;
        }
        
        auto lock = std::unique_lock<std::mutex>(m);
        console_input_queue.push_back(s);
        lock.unlock();
      }
      is_quit = true;
  });


  while (true)
  {
    uint32_t cur_time = enet_time_get();

    // Processing network input
    ENetEvent event;
    while (enet_host_service(client_host, &event, 10) > 0)
    {

      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        is_connected = true;

        break;
      case ENET_EVENT_TYPE_RECEIVE:

        // packet is from lobby
        if (event.peer->address == lobby_addr) {
          // packet with server data
          if (!is_started && enet_tool__is_server_data(event.packet)) {
            server_addr = enet_tool__parse_server_address(event.packet);
            server_peer = enet_tool__connect(server_addr, client_host);
            is_started = true;
          }

          // packet with lobby time
          if (enet_tool__is_systime(event.packet)) {
            uint8_t* data = event.packet->data + 1;
            uint32_t lobby_time = *(uint32_t*)data;
            printf("Lobby time: %u (diff=%d)\n", lobby_time, (int32_t)(lobby_time - cur_time));
          }
        }

        // packet is from server
        if (event.peer->address == server_addr) {
          // packet with server time
          if (enet_tool__is_systime(event.packet)) {
            uint8_t* data = event.packet->data + 1;
            uint32_t server_time = *(uint32_t*)data;
            printf("Server time: %u (diff=%d)\n", server_time, (int32_t)(server_time- cur_time));
          }

          // packet with ping list
          if (enet_tool__is_ping_list(event.packet)) {
            char* data = (char*)(event.packet->data + 1);
            printf("%s\n", data);
          }

          // packet with player list
          if (enet_tool__is_players_info(event.packet)) {
            printf("Server sent players list.\n");
            char* data = (char*)(event.packet->data + 1);
            printf("%s\n", data);
          }

          // packet with new player info
          if (enet_tool__is_player_info(event.packet)) {
            printf("New player connected\n");
            char* data = (char*)(event.packet->data + 1);
            printf("%s\n", data);
          }
        }
        

        printf("\n");
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    // Processing console input
    std::string console_input;
    bool has_input = false;

    auto lock = std::unique_lock<std::mutex>(m);
    if (!console_input_queue.empty()) {
      has_input = true;
      console_input = console_input_queue.back();
      console_input_queue.pop_back();
    }
    lock.unlock();
    

    if (is_connected)
    {
      if (has_input) {
        if (!is_started && console_input == START_MESSAGE) {
          send_start_packet(lobby_peer);
        }
      }
    }

    if (is_started) {
      if (cur_time > systime_send_timestamp)
      {
        systime_send_timestamp = cur_time + SYSTIME_SEND_INTERVAL_MS;
        send_systime_packet_unsequenced(server_peer);
      }
    }

  }
  return 0;
}
