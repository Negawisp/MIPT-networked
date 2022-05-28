#include "compression.h"
#include "quantisation.h"


void update_snapshot(Snapshot& snapshot, std::map<uint16_t, Entity>& entities)
{
  for (const auto& e : entities) {
    snapshot.entity_data[e.first] = entity_to_entity_snapshot(e.second);
  }
}

EntitySnapshot entity_to_entity_snapshot(const Entity& e)
{
  uint16_t xPacked = pack_float<uint16_t>(e.x, -16.f, 16.f, 11);
  uint16_t yPacked = pack_float<uint16_t>(e.y, -8.f, 8.f, 10);
  uint8_t oriPacked = pack_float<uint8_t>(e.ori, -PI, PI, 8);

  return EntitySnapshot{ xPacked, yPacked, oriPacked };
}