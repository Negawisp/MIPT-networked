// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/timer.h>
#include <debugdraw/debugdraw.h>
#include <functional>
#include "app.h"
#include <enet/enet.h>
#include <math.h>
#include <ctime>  
#include <vector>

//for scancodes
#include <GLFW/glfw3.h>

#include "quantisation.h"
#include "compression.h"
#include "entity.h"
#include "protocol.h"

static bool sign_correctly = true;

static std::map<uint16_t, Entity> entities;
static uint16_t my_entity = invalid_entity;

static uint8_t my_id = -1;
static uint32_t my_signature_key = 0;
static Snapshot ref_snapshot{};

void on_signature_data_received(ENetPacket* packet)
{
  SignatureData d;
  deserialize_signature_data(packet, d);
  my_id = d.client_id;
  my_signature_key = d.key;
}

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);

  if (0 < entities.count(newEntity.eid))
      return; // don't need to do anything, we already have entity
  entities[newEntity.eid] = newEntity;
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void on_ref_snapshot(ENetPacket* packet)
{
  uint16_t eid = invalid_entity;
  EntitySnapshot new_snapshot;
  deserialize_ref_snapshot(packet, eid, new_snapshot);

  ref_snapshot.entity_data[eid] = new_snapshot;
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float ori = 0.f;
  deserialize_snapshot(packet, ref_snapshot, eid, x, y, ori);

  if (ref_snapshot.entity_data.count(eid) == 0) {
      return;
  }
  entities[eid].x = x;
  entities[eid].y = y;
  entities[eid].ori = ori;
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 1920;
  int height = 1080;
  if (!app_init(width, height))
    return 1;
  ddInit();

  bx::Vec3 eye(0.f, 0.f, -16.f);
  bx::Vec3 at(0.f, 0.f, 0.f);
  bx::Vec3 up(0.f, 1.f, 0.f);

  float view[16];
  float proj[16];
  bx::mtxLookAt(view, bx::load<bx::Vec3>(&eye.x), bx::load<bx::Vec3>(&at.x), bx::load<bx::Vec3>(&up.x) );


  bool connected = false;
  int64_t now = bx::getHPCounter();
  int64_t last = now;
  float dt = 0.f;
  while (!app_should_close())
  {
    time_t t;
    static time_t q_pressed_t = 0;
    static time_t q_pressed_timeout = 1;
    time(&t);

    if (app_keypressed(GLFW_KEY_Q) && t - q_pressed_t > q_pressed_timeout) {
      q_pressed_t = t;
      sign_correctly = !sign_correctly;
    }

    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_SIGNATURE_KEY:
          on_signature_data_received(event.packet);
          break;
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          break;
        case E_SERVER_TO_CLIENT_REF_SNAPSHOT:
          on_ref_snapshot(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        break;
      default:
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      bool left = app_keypressed(GLFW_KEY_LEFT);
      bool right = app_keypressed(GLFW_KEY_RIGHT);
      bool up = app_keypressed(GLFW_KEY_UP);
      bool down = app_keypressed(GLFW_KEY_DOWN);
      // TODO: Direct adressing, of course!
      for (auto& e : entities)
        if (e.second.eid == my_entity)
        {
          // Update
          float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
          float steer = (left ? 1.f : 0.f) + (right ? -1.f : 0.f);

          // Send
          send_entity_input(sign_correctly, serverPeer, my_id, my_signature_key, my_entity, thr, steer);
        }
    }

    app_poll_events();
    // Handle window resize.
    app_handle_resize(width, height);
    bx::mtxProj(proj, 60.0f, float(width)/float(height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
    bgfx::setViewTransform(0, view, proj);

    // This dummy draw call is here to make sure that view 0 is cleared if no other draw calls are submitted to view 0.
    const bgfx::ViewId kClearView = 0;
    bgfx::touch(kClearView);

    DebugDrawEncoder dde;

    dde.begin(0);

    for (const auto &e : entities)
    {
      dde.push();

        dde.setColor(e.second.color);
        bx::Vec3 dir = {cosf(e.second.ori), sinf(e.second.ori), 0.f};
        bx::Vec3 pos = {e.second.x, e.second.y, -0.01f};
        dde.drawCapsule(sub(pos, dir), add(pos, dir), 1.f);

      dde.pop();
    }

    dde.end();

    // Advance to next frame. Process submitted rendering primitives.
    bgfx::frame();
    const double freq = double(bx::getHPFrequency());
    int64_t now = bx::getHPCounter();
    dt = (float)((now - last) / freq);
    last = now;
    printf("%f\n", 1.f/dt);
  }
  ddShutdown();
  bgfx::shutdown();
  app_terminate();
  return 0;
}
