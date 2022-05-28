#include "protocol.h"
#include "quantisation.h"
#include <cstring> // memcpy
#include <iostream>
#include <bit>

Key_t generate_key()
{
    return static_cast<Key_t>(rand());
}

Signature_t sign_data(Key_t key, const uint8_t* data_ptr, size_t data_size /*bits*/)
{
    Signature_t m = 0;
    for (int i = 0; i < data_size / 8; i++) {
        m = m ^ data_ptr[i];
    }
    m = m | data_size;
    int step = 4;
    for (int i = 0; i < sizeof(m) / step; i += step)
    {
        m = rotr<Signature_t>(m, i, i + step, step - 1);
        m = rotr<Signature_t>(m, 0, sizeof(m), 2);
        m = m ^ key;
    }
    return m;
}

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_signature_data(ENetPeer* peer, const SignatureData& data)
{
  ENetPacket* packet = enet_packet_create(nullptr, sizeof(MessageType) + sizeof(SignatureData),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t* ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SIGNATURE_KEY; ptr += sizeof(MessageType);
  memcpy(ptr, &data, sizeof(SignatureData)); ptr += sizeof(SignatureData);

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(MessageType) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_NEW_ENTITY; ptr += sizeof(MessageType);
  memcpy(ptr, &ent, sizeof(Entity)); ptr += sizeof(Entity);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(MessageType) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY; ptr += sizeof(MessageType);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

  enet_peer_send(peer, 0, packet);
}

void send_entity_input(bool sign_correctly, ENetPeer* peer, uint8_t client_id, Key_t signature_key, uint16_t eid, float thr, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(MessageType) +
                                                   sizeof(uint8_t)     + // player id
                                                   sizeof(Signature_t) + // packet signature
                                                   sizeof(uint16_t)    + // entity id
                                                   sizeof(uint8_t),      // compressed entity data
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  
  *ptr = E_CLIENT_TO_SERVER_INPUT; ptr += sizeof(MessageType);
  
  memcpy(ptr, &client_id, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  
  uint8_t* signature_ptr = ptr; ptr += sizeof(Signature_t); // Skipping to fill later
  uint8_t* data_ptr = ptr;
  
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  
  float4bitsQuantized thrPacked(thr, -1.f, 1.f);
  float4bitsQuantized oriPacked(ori, -1.f, 1.f);
  uint8_t thrSteerPacked = (thrPacked.packedVal << 4) | oriPacked.packedVal;
  memcpy(ptr, &thrSteerPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
  
  Signature_t signature = sign_correctly ? sign_data(signature_key, data_ptr, sizeof(uint16_t) + sizeof(uint8_t))
                                         : 0;
  memcpy(signature_ptr, &signature, sizeof(Signature_t));

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(MessageType) + sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  uint8_t *ptr = packet->data;
  *ptr = E_SERVER_TO_CLIENT_SNAPSHOT; ptr += sizeof(MessageType);
  memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  uint16_t xPacked = pack_float<uint16_t>(x, -16.f, 16.f, 11);
  uint16_t yPacked = pack_float<uint16_t>(y, -8.f, 8.f, 10);
  uint8_t oriPacked = pack_float<uint8_t>(ori, -PI, PI, 8);
  //printf("xPacked/unpacked %d %f\n", xPacked, x);
  memcpy(ptr, &xPacked, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  memcpy(ptr, &yPacked, sizeof(uint16_t)); ptr += sizeof(uint16_t);
  memcpy(ptr, &oriPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_signature_data(ENetPacket* packet, SignatureData& data)
{
  uint8_t* ptr = packet->data; ptr += sizeof(MessageType);
  data = *(SignatureData*)(ptr); ptr += sizeof(SignatureData);
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  uint8_t *ptr = packet->data; ptr += sizeof(MessageType);
  ent = *(Entity*)(ptr); ptr += sizeof(Entity);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  uint8_t *ptr = packet->data; ptr += sizeof(MessageType);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
}

bool deserialize_entity_input(ENetPacket *packet, std::map<uint8_t, uint32_t>& client_keys, uint16_t &eid, float &thr, float &steer)
{
  uint8_t *ptr = packet->data; ptr += sizeof(MessageType);

  uint8_t player_id = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  Signature_t received_signature = *(Signature_t*)(ptr); ptr += sizeof(Signature_t);

  uint8_t* data_ptr = ptr;
  size_t data_size = sizeof(uint16_t) + sizeof(uint8_t);
  Signature_t calculated_signature = sign_data(client_keys[player_id], data_ptr, data_size);
  if (received_signature != calculated_signature) {
    return false;
  }

  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  uint8_t thrSteerPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  static uint8_t neutralPackedValue = pack_float<uint8_t>(0.f, -1.f, 1.f, 4);
  float4bitsQuantized thrPacked(thrSteerPacked >> 4);
  float4bitsQuantized steerPacked(thrSteerPacked & 0x0f);
  thr = thrPacked.packedVal == neutralPackedValue ? 0.f : thrPacked.unpack(-1.f, 1.f);
  steer = steerPacked.packedVal == neutralPackedValue ? 0.f : steerPacked.unpack(-1.f, 1.f);

  return true;
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori)
{
  uint8_t *ptr = packet->data; ptr += sizeof(MessageType);
  eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  uint16_t xPacked = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  uint16_t yPacked = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
  uint8_t oriPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
  x = unpack_float<uint16_t>(xPacked, -16.f, 16.f, 11);
  y = unpack_float<uint16_t>(yPacked, -8.f, 8.f, 10);
  ori = unpack_float<uint8_t>(oriPacked, -PI, PI, 8);
}

