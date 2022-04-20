#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <cstring>
#include "common/constants.h"
#include "common/enet_tools.h"

void send_server_data_packet(ENetPeer *peer, const ENetAddress* server_address)
{
  uint8_t* message_buff = (uint8_t*)calloc(1, sizeof(ENetAddress) + 1);
  message_buff[0] = FLAG_IS_SERVER_DATA;
  *(ENetAddress*)(message_buff + 1) = *server_address;

  ENetPacket *packet = enet_packet_create(message_buff, sizeof(ENetAddress) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 1, packet);
  
  free(message_buff);
}

int main(int argc, const char **argv)
{
  bool is_started = false;

  uint32_t time_start = enet_time_get();
  uint32_t systime_send_timestamp = time_start;

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  atexit(enet_deinitialize);

  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = DEFAULT_LOBBY_PORT;
  ENetHost *lobby_host = enet_host_create(&address, 32, 2, 0, 0);

  if (!lobby_host)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }
  
  ENetAddress server_address;
  enet_address_set_host(&server_address, "localhost");
  server_address.port = DEFAULT_SERVER_PORT;

  std::vector<ENetPeer*> clients;

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(lobby_host, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        clients.push_back(event.peer);

        if (is_started) {
          send_server_data_packet(event.peer, &server_address);
        }

        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data + 1);
        
        // Received "Start" from a player
        if (!is_started && enet_tool__is_start(event.packet)) {
          is_started = true;
          for (ENetPeer* client : clients) {
            send_server_data_packet(client, &server_address);
          }
        }
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }


    // Sending systime
    uint32_t cur_time = enet_time_get();
    if (cur_time > systime_send_timestamp)
    {
      systime_send_timestamp = cur_time + SYSTIME_SEND_INTERVAL_MS * 10;
      for (auto player_peer : clients) {
        send_systime_packet_unsequenced(player_peer);
      }
    }
  }

  enet_host_destroy(lobby_host);

  return 0;
}

