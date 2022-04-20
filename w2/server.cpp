#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>

#include "common/enet_tools.h"
#include "common/constants.h"

const int NAMES_VARIANTS = 5;
const int NAME_MAX_LEN = 32;


struct PlayerData {
  uint32_t id;
  std::string name;
  uint32_t ping;
};

const std::string FIRST_NAMES[NAMES_VARIANTS] = {
  "John",
  "Chris",
  "Elliot",
  "Perry",
  "Carla"
};

const std::string SECOND_NAMES[NAMES_VARIANTS] = {
  "Dorian",
  "Turk",
  "Reid",
  "Cox",
  "Espinosa"
};

std::string generate_name() {
  return FIRST_NAMES[std::rand() % NAMES_VARIANTS] + SECOND_NAMES[std::rand() % NAMES_VARIANTS];
}


struct AddrComparator {
  bool operator()(const ENetAddress& x, const ENetAddress& y) const {
    return x.host < y.host || x.host == y.host && x.port < y.port;
  }
};

void send_player_list_packet(ENetPeer *peer, std::map<ENetAddress, PlayerData, AddrComparator> player_datas) { 
  std::string message;
  message.push_back(FLAG_IS_PLAYER_LIST);
  message.append("Players on server (")
         .append(std::to_string(player_datas.size()))
         .append(" connected):\n");
  for (auto pair : player_datas) {
    message.append("Id: ").append(std::to_string(pair.second.id))
           .append(", Name: ").append(pair.second.name)
           .append("\n");
  }
  
  ENetPacket *packet = enet_packet_create(message.data(), message.size() + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 1, packet);
}

void send_new_player_packet(ENetPeer *peer, PlayerData player_data) {
  std::string message;
  message.push_back(FLAG_IS_NEW_PLAYER);
  message.append("New player connected:\n");
  message.append("Id: ").append(std::to_string(player_data.id))
         .append(", Name: ").append(player_data.name)
         .append("\n");
  
  ENetPacket *packet = enet_packet_create(message.data(), message.size() + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 1, packet);
} 

void send_ping_list_packet(ENetPeer *peer, std::map<ENetAddress, PlayerData, AddrComparator> player_datas) {
  std::string message;
  message.push_back(FLAG_IS_PING_LIST);
  message.append("PlayerId : ping:\n");
  for (auto pair : player_datas) {
    message.append(std::to_string(pair.second.id))
           .append(" : ").append(std::to_string(pair.second.ping))
           .append("\n");
  }

  ENetPacket *packet = enet_packet_create(message.data(), message.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}

int main(int argc, const char **argv)
{
  uint32_t time_start = enet_time_get();
  uint32_t systime_send_timestamp = time_start;
  uint32_t ping_send_timestamp = time_start;

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  atexit(enet_deinitialize);

  ENetAddress address;
  address.host = ENET_HOST_ANY;
  address.port = DEFAULT_SERVER_PORT;
  ENetHost *server_host = enet_host_create(&address, 32, 2, 0, 0);

  if (!server_host)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  std::vector<ENetPeer*> player_peers;
  std::map<ENetAddress, PlayerData, AddrComparator> player_datas;


  while (true)
  {
    uint32_t cur_time = enet_time_get();

    ENetEvent event;
    while (enet_host_service(server_host, &event, 10) > 0)
    {
      PlayerData player_data;
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);

        send_player_list_packet(event.peer, player_datas);

        player_data.id = player_datas.size();
        player_data.name = generate_name();

        for (auto player_peer : player_peers) {
          send_new_player_packet(player_peer, player_data);
        }

        player_datas[event.peer->address] = player_data;
        player_peers.push_back(event.peer);

        break;
      case ENET_EVENT_TYPE_RECEIVE:
        // Packet must be from a user
        if (player_datas.find(event.peer->address) == player_datas.end())
          break;

        player_data = player_datas[event.peer->address];
        printf("id%u %s says:\n", player_data.id, player_data.name.c_str());

        // Packet is player's systime
        if (enet_tool__is_systime(event.packet)) {
          uint8_t* data = event.packet->data + 1;
          uint32_t client_time = *(uint32_t*)data;
          printf("Client time: %u (diff=%d)\n", client_time, (int32_t)(client_time - cur_time));
        }

        printf("\n");

        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    // Sending systime
    if (cur_time > systime_send_timestamp)
    {
      systime_send_timestamp = cur_time + SYSTIME_SEND_INTERVAL_MS;
      for (auto player_peer : player_peers) {
        send_systime_packet_unsequenced(player_peer);
      }
    }

    // Sending pings
    if (cur_time > ping_send_timestamp)
    {
      ping_send_timestamp = cur_time + PING_SEND_INTERVAL_MS;
      for (auto player_peer : player_peers) {
        player_datas[player_peer->address].ping = player_peer->pingInterval;
      }
      for (auto player_peer : player_peers) {
        send_ping_list_packet(player_peer, player_datas);
      }
    }

    std::cout << std::flush;
  }

  enet_host_destroy(server_host);

  return 0;
}
