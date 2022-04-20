#pragma once

#include <enet/enet.h>
#include "constants.h"

ENetPeer* enet_tool__connect(ENetAddress& address, ENetHost *this_host);
ENetPeer* enet_tool__connect(enet_uint32 host, enet_uint16 port, ENetHost *this_host);
ENetPeer* enet_tool__connect_local(enet_uint16 port, ENetHost *this_host);

bool enet_tool__is_start(const ENetPacket* packet);
bool enet_tool__is_systime(const ENetPacket* packet);
bool enet_tool__is_ping_list(const ENetPacket* packet);
bool enet_tool__is_server_data(const ENetPacket* packet);
ENetAddress enet_tool__parse_server_address(const ENetPacket* packet);

void send_systime_packet_unsequenced(ENetPeer *peer);

bool operator==(const ENetAddress& x, const ENetAddress& y);
