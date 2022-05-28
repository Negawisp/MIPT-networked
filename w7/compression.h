#pragma once

#include <stdint.h>
#include <vector>
#include <map>
#include <assert.h>
#include "entity.h"

typedef uint8_t CompressionBits_t;

template<typename T> // sizeof(T) must be <= sizeof(CompressionBits_t) * 8.
                     // This is required to be able to tell which bytes of data differ from snapshot_data.
class CompressedData {
public:
  CompressedData(const T* data, const T* ref_data);
  ~CompressedData();

  uint8_t* get_data() { return m_data.data(); }
  size_t get_size() { return sizeof(uint8_t) * m_data.size(); }
  CompressionBits_t get_compression_bits() { return m_compression_bits; }

private:
  std::vector<uint8_t> m_data;
  CompressionBits_t m_compression_bits;
};


template<typename T>
CompressedData<T>::CompressedData(const T* data, const T* ref_data)
{
    uint8_t* p_data = (uint8_t*)data;
    uint8_t* p_ref_data = (uint8_t*)ref_data;

    m_compression_bits = 0;
    for (int i = 0; i < sizeof(T); i++) {
        if (p_data[i] != p_ref_data[i]) {
            m_compression_bits = m_compression_bits | (1 << i);
            m_data.push_back(p_data[i]);
        }
    }
}

template<typename T>
CompressedData<T>::~CompressedData()
{
}


template<typename T>
T decompress_data(const uint8_t* data, const T* ref_data, CompressionBits_t compression_bits)
{
    assert(sizeof(CompressionBits_t) * 8 >= sizeof(T));

    T ret_data{};
    uint8_t* p_ret_data = (uint8_t*)&ret_data;
    uint8_t* p_ref_data = (uint8_t*)ref_data;

    int k = 0;
    for (int i = 0; i < sizeof(T); i++) {
        if ((compression_bits & 1) == 1) {
          p_ret_data[i] = data[k++];
        }
        else {
          p_ret_data[i] = p_ref_data[i];
        }
        compression_bits = compression_bits >> 1;
    }

    /*
    int I = sizeof(T) / 8;
    for (int i = 0; i < I; i++) {
      p_ret_data[i] = p_ref_data[i];
    }
    */

    return ret_data;
}

struct EntitySnapshot {
  uint16_t x_packed;
  uint16_t y_packed;
  uint8_t ori_packed;
};
struct Snapshot {
  std::map<uint16_t, EntitySnapshot> entity_data;
};
const uint32_t SNAPSHOT_TIME_NOT_SET = ~0;

void update_snapshot(Snapshot& snapshot, std::map<uint16_t, Entity>& entities);
EntitySnapshot entity_to_entity_snapshot(const Entity& e);
