#pragma once
#include <enet/enet.h>
#include <cstdint>
#include <map>
#include "entity.h"
#include "compression.h"

enum MessageType : uint8_t
{
  E_CLIENT_TO_SERVER_JOIN = 0,
  E_SERVER_TO_CLIENT_SIGNATURE_KEY,
  E_SERVER_TO_CLIENT_NEW_ENTITY,
  E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY,
  E_CLIENT_TO_SERVER_INPUT,
  E_SERVER_TO_CLIENT_REF_SNAPSHOT,
  E_SERVER_TO_CLIENT_SNAPSHOT
};

typedef uint16_t Signature_t;
typedef uint16_t Key_t;
struct SignatureData {
  uint8_t client_id;
  Key_t key;
};

struct PlayerData {
  uint8_t client_id;
  ENetPeer* peer;
  Key_t key;
  uint32_t snapshot_time;
  uint32_t new_snapshot_time;
  bool new_snapshot_time_acked;
};

Key_t generate_key();
Signature_t sign_data(Key_t key, const uint8_t* data_ptr, size_t data_size);

void send_join(ENetPeer *peer);
void send_initial_data(ENetPeer* peer, const SignatureData& data);
void send_new_entity(ENetPeer *peer, const Entity &ent);
void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_entity_input(bool sign_correctly, ENetPeer *peer, uint8_t client_id, Key_t signature_key, uint16_t eid, float thr, float steer);
void send_ref_snapshot(PlayerData& player_data, uint16_t eid, EntitySnapshot ref_snapshot);
void send_snapshot(PlayerData& player_data, EntitySnapshot reference_snapshot, uint16_t eid, float x, float y, float ori);


MessageType get_packet_type(ENetPacket *packet);

void deserialize_new_entity(ENetPacket *packet, Entity &ent);
void deserialize_signature_data(ENetPacket* packet, SignatureData& data);
void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
/* Returns: wether the packet is signed correctly */
bool deserialize_entity_input(ENetPacket *packet, std::map<uint8_t, PlayerData>& client_keys, uint16_t &eid, float &thr, float &steer);
void deserialize_ref_snapshot(ENetPacket* packet, uint16_t& out_eid, EntitySnapshot& out_ref_snapshot);
void deserialize_snapshot(ENetPacket* packet, Snapshot& ref_snapshot, uint16_t& eid, float& x, float& y, float& ori);
