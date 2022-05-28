#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include "compression.h"
#include "mathUtils.h"
#include <thread>
#include <stdlib.h>
#include <vector>
#include <map>

static Snapshot snapshot;
static uint32_t snapshot_updated_timestamp = 0;
static const uint32_t SNAPSHOT_UPDATE_PERIOD = 2000;

static std::map<uint8_t, PlayerData> player_data;
static std::map<uint16_t, Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;

void on_client_connected(ENetPeer* peer) {
  uint8_t clients_count = static_cast<uint8_t>(player_data.size());
  Key_t key = generate_key();

  player_data[clients_count] = PlayerData{
    clients_count,
    peer,
    key,
    SNAPSHOT_TIME_NOT_SET,
    SNAPSHOT_TIME_NOT_SET,
    false
  };

  send_initial_data(peer, SignatureData{clients_count, key});
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const auto &ent : entities)
    send_new_entity(peer, ent.second);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const auto &e : entities)
    maxEid = std::max(maxEid, e.second.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0xff000000 +
                   0x00440000 * (rand() % 5) +
                   0x00004400 * (rand() % 5) +
                   0x00000044 * (rand() % 5);
  float x = (rand() % 4) * 2.f;
  float y = (rand() % 4) * 2.f;
  Entity ent = {color, x, y, 0.f, (rand() / RAND_MAX) * 3.141592654f, 0.f, 0.f, newEid};
  entities[newEid] = (ent);

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_input(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float thr = 0.f; float steer = 0.f;
  if (false == deserialize_entity_input(packet, player_data, eid, thr, steer)) {
    return;
  }
  entities[eid].thr = thr;
  entities[eid].steer = steer;
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  uint32_t lastTime = enet_time_get();
  while (true)
  {
    uint32_t curTime = enet_time_get();
    float dt = (curTime - lastTime) * 0.001f;
    lastTime = curTime;

    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        on_client_connected(event.peer);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_INPUT:
            on_input(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    bool should_send_ref = false;
    if (curTime > snapshot_updated_timestamp) {
      snapshot_updated_timestamp = curTime + SNAPSHOT_UPDATE_PERIOD;
      update_snapshot(snapshot, entities);
      should_send_ref = true;
    }

    static int t = 0;
    for (auto &e : entities)
    {
      // simulate
      simulate_entity(e.second, dt);
      // send

      if (should_send_ref) {
        for (auto& player_pair : player_data)
        {
          send_ref_snapshot(player_pair.second, e.first, snapshot.entity_data[e.first]);
        }
      }

      for (auto& player_pair : player_data)
      {
        send_snapshot(player_pair.second, snapshot.entity_data[e.first], e.first, e.second.x, e.second.y, e.second.ori);
      }
    }
    std::this_thread::sleep_for(std::chrono::microseconds(123));
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


