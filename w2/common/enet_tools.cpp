#include "enet_tools.h"

#include <stdio.h>
#include <string>

ENetPeer* enet_tool__connect(ENetAddress& address, ENetHost *this_host) {
  ENetPeer *lobbyPeer = enet_host_connect(this_host, &address, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
  }
  return lobbyPeer;
}

ENetPeer* enet_tool__connect_local(enet_uint16 port, ENetHost *this_host) {
  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = port;

  return enet_tool__connect(address, this_host);
}

ENetPeer* enet_tool__connect(enet_uint32 host, enet_uint16 port, ENetHost *this_host) {
  ENetAddress address;
  address.host = host;
  address.port = port;

  return enet_tool__connect(address, this_host);
}


bool enet_tool__is_start(const ENetPacket* packet) {
  return packet->data[0] & FLAG_IS_START;
}

bool enet_tool__is_systime(const ENetPacket* packet) {
    return packet->data[0] & FLAG_IS_SYSTIME;
}

bool enet_tool__is_ping_list(const ENetPacket* packet) {
    return packet->data[0] & FLAG_IS_PING_LIST;
}

bool enet_tool__is_server_data(const ENetPacket* packet) {
  return packet->data[0] & FLAG_IS_SERVER_DATA;
}

ENetAddress enet_tool__parse_server_address(const ENetPacket* packet) {
  return *(ENetAddress*)(packet->data + 1);
}

void send_systime_packet_unsequenced(ENetPeer *peer) {
  uint32_t time_to_send = enet_time_get() + static_cast<uint32_t>(std::rand() % 500);
  size_t msg_size = sizeof(uint32_t) + 1;

  uint8_t* msg = (uint8_t*)calloc(1, msg_size);
  msg[0] = FLAG_IS_SYSTIME;
  *(uint32_t*)(msg + 1) = time_to_send;

  ENetPacket *packet = enet_packet_create(msg, msg_size, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);

  free(msg);
}

bool operator==(const ENetAddress& x, const ENetAddress& y) {
  return x.host == y.host && x.port == y.port;
}